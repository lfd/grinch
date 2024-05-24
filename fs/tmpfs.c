/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ern Lim <ern.lim@st.oth-regensburg.de>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/fs/tmpfs.h>
#include <grinch/fs/util.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

#define TMPFS_DIR	(1 << 0)
#define TMPFS_CREATE	(1 << 1)

struct tmpfs_entry {
	struct list_head files;

	bool is_dir;

	char *name;
	union {
		void *raw;
		struct list_head files;
	} content;

	unsigned int size;

	spinlock_t lock;
};

static ssize_t tmpfs_read(struct file_handle *h, char *ubuf, size_t count)
{
	struct tmpfs_entry *file;
	ssize_t ret;
	size_t left;
	void *raw;

	if ((ssize_t)count < 0)
		return -EFBIG;

	if (h->flags.is_kernel)
		return -ENOSYS;

	file = h->fp->drvdata;
	spin_lock(&file->lock);

	if (file->is_dir) {
		ret = -EBADF;
		goto unlock_out;
	}

	raw = file->content.raw;
	if (!raw) {
		ret = 0;
		goto unlock_out;
	}

	/* must not happen */
	if (h->position > file->size)
		BUG();
	left = file->size - h->position;

	ret = copy_to_user(current_task(), ubuf, raw + h->position,
			   MIN(count, left));
	h->position += ret;

unlock_out:
	spin_unlock(&file->lock);

	return ret;
}

static ssize_t tmpfs_write(struct file_handle *h, const char *buf, size_t count)
{
	struct tmpfs_entry *file;
	void *tmp_content, *raw;
	unsigned long copied;
	ssize_t ret;

	if (!count)
		return 0;

	if ((ssize_t)count < 0)
		return -EFBIG;

	if (h->flags.is_kernel)
		return -ENOSYS;

	file = h->fp->drvdata;
	spin_lock(&file->lock);

	if (file->is_dir) {
		ret = -EBADF;
		goto unlock_out;
	}

	loff_t offset = h->position;

	raw = file->content.raw;
	if (offset + count > file->size) {
		tmp_content = kmalloc(offset + count);
		if (!tmp_content){
			ret = -ENOMEM;
			goto unlock_out;
		}

		memcpy(tmp_content, raw, offset);
		kfree(raw);
		raw = file->content.raw = tmp_content;
		file->size = offset + count;
	}

	copied = copy_from_user(current_task(), raw + offset, buf, count);

	if (count == copied)
		ret = (ssize_t)copied;
	else {
		memset(raw + offset + copied, 0, count - copied);
		ret = -EFAULT;
	}
	h->position += copied;

unlock_out:
	spin_unlock(&file->lock);

	return ret;
}

static int tmpfs_getdents(struct file_handle *h, struct grinch_dirent *udents,
			  unsigned int size)
{
	struct tmpfs_entry *dir, *file;
	struct grinch_dirent dent;
	unsigned int index;
	int err;

	dir = h->fp->drvdata;
	spin_lock(&dir->lock);
	if (!dir->is_dir)
		BUG();

	index = 0;
	file = NULL;
	list_for_each_entry(file, &dir->content.files, files) {
		if (index == h->position)
			goto found;
		index++;
	}

	err = 0;
	goto unlock_out;

found:
	dent.type = file->is_dir ? DT_DIR : DT_REG;
	err = copy_dirent(udents, h->flags.is_kernel, &dent, file->name, size);
	if (err)
		return err;

	err = 1;
	h->position += 1;

unlock_out:
	spin_unlock(&dir->lock);
	return err;
}

static struct tmpfs_entry *
find_create_entry(struct tmpfs_entry *dir, const char *path, unsigned int flags)
{
	struct tmpfs_entry *entry;
	bool directory, create;

	directory = !!(flags & TMPFS_DIR);
	create = !!(flags & TMPFS_CREATE);

	list_for_each_entry(entry, &dir->content.files, files) {
		if (strcmp(entry->name, path))
			continue;

		if (directory && create)
			return ERR_PTR(-EEXIST);

		return entry;
	}

	if (!create)
		return ERR_PTR(-ENOENT);

	/* entry shall be created */
	entry = kzalloc(sizeof(*entry));
	if (!entry)
		return ERR_PTR(-ENOMEM);

	spin_init(&entry->lock);

	entry->name = kstrdup(path);
	if (!entry->name) {
		kfree(entry);
		return ERR_PTR(-ENOMEM);
	}

	if (directory) {
		INIT_LIST_HEAD(&entry->content.files);
		entry->is_dir = true;
	}

	list_add(&entry->files, &dir->content.files);

	return entry;
}

static int tmpfs_stat(struct file *_file, struct stat *st)
{
	struct tmpfs_entry *file;

	file = _file->drvdata;

	if (file->is_dir) {
		st->st_mode = S_IFDIR;
		return 0;
	}

	st->st_mode = S_IFREG;
	st->st_size = file->size;

	return 0;
}

static const struct file_operations tmpfs_fops;

static int tmpfs_lookup_entry(struct file *_dir, struct file *file, const char *name, unsigned int flags)
{
	struct tmpfs_entry *dir, *entry;
	int err;

	dir = _dir->drvdata;
	spin_lock(&dir->lock);

	entry = find_create_entry(dir, name, flags);
	if (IS_ERR(entry)) {
		err = PTR_ERR(entry);
		goto unlock_out;
	}

	file->fops = &tmpfs_fops;
	file->drvdata = entry;
	file->is_directory = entry->is_dir;
	err = 0;

unlock_out:
	spin_unlock(&dir->lock);
	return err;

}

static int tmpfs_create(struct file *dir, struct file *file, const char *name,
			mode_t mode)
{
	return tmpfs_lookup_entry(dir, file, name, TMPFS_CREATE);
}

static int tmpfs_mkdir(struct file *dir, struct file *file, const char *name,
		       mode_t mode)
{
	return tmpfs_lookup_entry(dir, file, name, TMPFS_CREATE | TMPFS_DIR);
}

static int tmpfs_open(struct file *dir, struct file *file, const char *name)
{
	return tmpfs_lookup_entry(dir, file, name, 0);
};

static int tmpfs_mount(const struct file_system *fs, struct file *dir)
{
	struct tmpfs_entry *root;

	root = fs->drvdata;

	dir->is_directory = root->is_dir;
	dir->fops = &tmpfs_fops;
	dir->drvdata = root;

	return 0;
}

static const struct file_system_operations tmpfs_ops = {
	.open_file = tmpfs_open,
	.mount = tmpfs_mount,
};

int tmpfs_new(struct file_system *fs)
{
	struct tmpfs_entry *root;

	root = kzalloc(sizeof(*root));
	if (!root)
		return -ENOMEM;

	spin_init(&root->lock);
	root->is_dir = true;
	INIT_LIST_HEAD(&root->content.files);

	fs->fs_ops = &tmpfs_ops;
	fs->drvdata = root;

	return 0;
}

static const struct file_operations tmpfs_fops = {
	.read = tmpfs_read,
	.write = tmpfs_write,
	.stat = tmpfs_stat,
	.create = tmpfs_create,
	.getdents = tmpfs_getdents,
	.mkdir = tmpfs_mkdir,
};

const struct file_system tmpfs = {
        .fs_ops = &tmpfs_ops,
};
