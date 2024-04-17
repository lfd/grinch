/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"vfs: " x

#include <asm-generic/fcntl.h>
#include <asm/spinlock.h>

#include <string.h>

#include <grinch/alloc.h>
#include <grinch/fs/devfs.h>
#include <grinch/fs/initrd.h>
#include <grinch/fs/vfs.h>
#include <grinch/errno.h>
#include <grinch/list.h>
#include <grinch/kstr.h>
#include <grinch/panic.h>
#include <grinch/printk.h>

static LIST_HEAD(open_files);
static DEFINE_SPINLOCK(files_lock);

struct files {
	struct list_head files;

	const char *pathname;
	struct file fp;
	unsigned int refs;
};

void file_get(struct file *file)
{
	struct files *files;

	files = container_of(file, struct files, fp);
	spin_lock(&files_lock);
	files->refs++;
	spin_unlock(&files_lock);
}

/* must hold files_lock */
static struct file *search_file(const char *path, struct fs_flags flags)
{
	struct files *files;

	list_for_each_entry(files, &open_files, files) {
		if (strcmp(path, files->pathname))
			continue;

		if ((flags.may_read && !files->fp.fops->read) ||
		    (flags.may_write && !files->fp.fops->write)) {
			return ERR_PTR(-EINVAL);
		}

		files->refs++;
		return &files->fp;
	}

	return ERR_PTR(-ENOENT);
}

/* must hold files_lock */
static struct file *_file_open(const char *pathname, struct fs_flags flags)
{
	const struct file_system *fs;
	struct files *files;
	const char *fsname;
	int err;

	fsname = pathname;
	fs = NULL;
	if (!strncmp(pathname, DEVFS_MOUNTPOINT, DEVFS_MOUNTPOINT_LEN)) {
		fs = &devfs;
		fsname += 5;
	} else if (!strncmp(fsname, "/initrd/", 8)) {
		fs = &initrdfs;
		fsname += 8;
	}

	if (!fs)
		return ERR_PTR(-ENOENT);

	files = kzalloc(sizeof(*files));
	if (!files)
		return ERR_PTR(-ENOMEM);

	files->pathname = kstrdup(pathname);
	if (!files->pathname) {
		kfree(files);
	}

	err = fs->fs_ops->open_file(fs, &files->fp, fsname, flags);
	if (err) {
		kfree(files->pathname);
		kfree(files);
		return ERR_PTR(err);
	}

	files->refs = 1;
	list_add(&files->files, &open_files);

	return &files->fp;
}

struct file *file_open(const char *path, struct fs_flags flags)
{
	struct file *filep;

	spin_lock(&files_lock);
	filep = search_file(path, flags);
	if (filep != ERR_PTR(-ENOENT))
		goto unlock_out;

	filep = _file_open(path, flags);

unlock_out:
	spin_unlock(&files_lock);

	return filep;
}

void file_close(struct file_handle *handle)
{
	struct files *files;
	struct file *fp;

	fp = handle->fp;
	files = container_of(fp, struct files, fp);

	if (!fp->fops)
		BUG();

	spin_lock(&files_lock);
	files->refs--;
	if (files->refs == 0) {
		if (fp->fops->close)
			fp->fops->close(fp);
		kfree(files->pathname);
		list_del(&files->files);
		kfree(files);
	}
	spin_unlock(&files_lock);

	handle->fp = NULL;
}

int check_path(const char *path)
{
	unsigned int no;

	/* no relative paths supported */
	if (path[0] != '/')
		return -EINVAL;

	for (no = 1; path[no]; no++) {
		/* no double slashes */
		if (path[no] == '/' && path[no - 1] == '/')
			return -EINVAL;
		/* no . files */
		else if (path[no] == '.' && path[no - 1] == '/')
			return -EINVAL;
	}

	return 0;
}

struct fs_flags get_flags(int oflag)
{
	struct fs_flags ret;

	ret.is_kernel = false;
	switch (oflag & O_ACCMODE) {
		case O_RDONLY:
			ret.may_read = true;
			ret.may_write = false;
			break;

		case O_WRONLY:
			ret.may_read = false;
			ret.may_write = true;
			break;

		case O_RDWR:
			ret.may_read = true;
			ret.may_write = true;
			break;
	}

	if (oflag & O_NONBLOCK)
		ret.nonblock = true;

	return ret;
}

void *vfs_read_file(const char *pathname, size_t *len)
{
	struct file_handle handle = { 0 };
	struct stat st;
	ssize_t err;
	void *ret;

	handle.flags.is_kernel = true;
	handle.fp = file_open(pathname, handle.flags);
	if (IS_ERR(handle.fp))
		return handle.fp;

	if (!handle.fp->fops->stat) {
		ret = ERR_PTR(-ENOSYS);
		goto close_out;
	}

	err = handle.fp->fops->stat(handle.fp, &st);
	if (err) {
		ret = ERR_PTR(err);
		goto close_out;
	}

	ret = kmalloc(st.st_size);
	if (!ret) {
		ret = ERR_PTR(-ENOMEM);
		goto close_out;
	}

	err = handle.fp->fops->read(&handle, ret, st.st_size);
	if (err < 0) {
		kfree(ret);
		ret = ERR_PTR(err);
		goto close_out;
	}

	if (len)
		*len = err;

close_out:
	file_close(&handle);

	return ret;
}

int __init vfs_init(void)
{
	int err;

	err = initrd_init();
	if (err == -ENOENT)
		pri("No ramdisk found\n");
	else if (err)
		return err;

	err = devfs_init();
	if (err)
		return err;

	return 0;
}
