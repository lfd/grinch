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

#include <asm/spinlock.h>

#include <grinch/alloc.h>
#include <grinch/fs/devfs.h>
#include <grinch/fs/initrd.h>
#include <grinch/fs/vfs.h>
#include <grinch/errno.h>
#include <grinch/list.h>
#include <grinch/panic.h>
#include <grinch/printk.h>

/*
 * When looking up dflc entries, having D_DIR set as flag means:
 *   - Entry must be a directory, if D_CREATE is not set
 *   - Entry will get a directory, if D_CREATE is set
 *
 * If D_DIR is cleared, D_CREATE means:
 *   - Entry must be a file, if D_CREATE is not set
 *   - Entry will be created as file, if D_CREATE is set
 */
#define D_DIR		(1 << 0)
#define D_CREATE	(1 << 1)

#define D_ISDIR(x)	!!((x) & D_DIR)
#define D_ISCREATE(x)	!!((x) & D_CREATE)

/* Directory & File Lookup Cache */
struct dflc {
	struct list_head siblings;

	struct dflc *parent;

	const char *name;

	/* children must only be used, if is_directory = true */
	struct list_head children;

	const struct file_system *fs;
	struct file fp;

	spinlock_t lock;
	unsigned int refs;
};

static struct dflc root = {
	.name = "",
	.siblings = LIST_HEAD_INIT(root.siblings),
	.children = LIST_HEAD_INIT(root.children),
	.refs = 1, // prevent closing
	.fp = {
		.is_directory = true,
	},
	.lock = SPIN_LOCK_UNLOCKED,
};

static struct dflc dev = {
	.parent = &root,
	.siblings = LIST_HEAD_INIT(dev.siblings),
	.name = "dev",
	.refs = 1, // prevent closing
	.children = LIST_HEAD_INIT(dev.children),
	.fs = &devfs,
	.lock = SPIN_LOCK_UNLOCKED,
};

static struct dflc ird = {
	.parent = &root,
	.siblings = LIST_HEAD_INIT(ird.siblings),
	.name = "initrd",
	.refs = 1, // prevent closing
	.children = LIST_HEAD_INIT(ird.children),
	.fs = &initrdfs,
	.lock = SPIN_LOCK_UNLOCKED,
};

static inline void dflc_lock(struct dflc *entry)
{
	spin_lock(&entry->lock);
}

static inline void dflc_unlock(struct dflc *entry)
{
	spin_unlock(&entry->lock);
}

static struct dflc *dflc_get(struct dflc *entry)
{
	dflc_lock(entry);
	entry->refs++;
	dflc_unlock(entry);

	return entry;
}

static void dflc_put(struct dflc *entry)
{
	struct dflc *parent;
	struct file *fp;

	fp = &entry->fp;

	if (entry->refs == 0)
		BUG();

	dflc_lock(entry);
	parent = entry->parent;
	entry->refs--;
	if (entry->refs == 0) {
		if (fp->fops && fp->fops->close)
			fp->fops->close(fp);

		if (entry != &root) {
			kfree(entry->name);
			list_del(&entry->siblings);
			kfree(entry);
		}
	}
	dflc_unlock(entry);

	if (parent)
		dflc_put(parent);
}

static inline struct dflc *dflc_of(struct file *file)
{
	return container_of(file, struct dflc, fp);
}

void file_dup(struct file *file)
{
	struct dflc *entry;

	entry = dflc_of(file);
	do {
		dflc_get(entry);
		entry = entry->parent;
	} while (entry);
}

void file_close(struct file *file)
{
	struct dflc *entry;

	entry = dflc_of(file);
	dflc_put(entry);
}

/*
 * Lookup, or possibly create the next dflc entry. If an entry exists, this
 * routine will not check against D_DIR. The caller must do this.
 *
 * We must arrive here with the parent's lock being held.
 */
static inline struct dflc *
dflc_lookup_next(struct dflc *parent, const char *_name, size_t n,
		 unsigned int flags)
{
	int (*fun)(struct file *, struct file *, const char *, mode_t);
	bool directory, create;
	struct dflc *next;
	char *name;
	int err;

	name = kstrndup(_name, n);
	if (!name)
		return ERR_PTR(-ENOMEM);

	list_for_each_entry(next, &parent->children, siblings)
		if (!strcmp(next->name, name)) {
			kfree(name);
			goto found;
		}

	next = kzalloc(sizeof *next);
	if (!next) {
		kfree(name);
		return ERR_PTR(-ENOMEM);
	}
	next->name = name;

	directory = D_ISDIR(flags);
	create = D_ISCREATE(flags);

	if (directory && create) {
		fun = parent->fp.fops->mkdir;
		goto fun_out;
	}

	if (!parent->fs->fs_ops || !parent->fs->fs_ops->open_file) {
		err = -ENOSYS;
		goto err_free_out;
	}

	err = parent->fs->fs_ops->open_file(&parent->fp, &next->fp, next->name);
	if (!err)
		goto opened;

	if (err != -ENOENT)
		goto err_free_out;

	if (!create)
		goto err_free_out;

	if (!parent->fp.fops) {
		err = -ENOSYS;
		goto err_free_out;
	}

	fun = parent->fp.fops->create;

fun_out:
	if (!fun) {
		err = -ENOSYS;
		goto err_free_out;
	}

	err = fun(&parent->fp, &next->fp, next->name, 0);
	if (err)
		goto err_free_out;

opened:
	spin_init(&next->lock);
	next->parent = parent;
	next->fs = parent->fs;
	list_add(&next->siblings, &parent->children);
	INIT_LIST_HEAD(&next->children);

found:
	dflc_get(next);
	return next;

err_free_out:
	kfree(next->name);
	kfree(next);
	return ERR_PTR(err);
}

/* Lookup, or possibly create the dflc entry pathname */
static struct dflc *dflc_lookup(const char *pathname, unsigned int _flags)
{
	struct dflc *parent, *next;
	const char *start, *end;
	unsigned int flags;
	int err;

	if (pathname[0] != '/')
		return ERR_PTR(-EINVAL);

	parent = dflc_get(&root);
	if (pathname[1] == '\0')
		return parent;

	/* Walk through all directories in between */
	start = end = &pathname[1];
	flags = D_DIR;
	do {
		end = strchrnul(start, '/');
		if (*end == '\0')
			flags = _flags;

		dflc_lock(parent);
		next = dflc_lookup_next(parent, start, end - start, flags);
		dflc_unlock(parent);
		if (IS_ERR(next)) {
			err = PTR_ERR(next);
			goto put_out;
		}

		parent = next;
		if (D_ISDIR(flags) && !parent->fp.is_directory) {
			err = -ENOTDIR;
			goto put_out;
		}

		if (*end == '\0')
			return parent;

		start = end + 1;
	} while (1);
	/* never reached */
	BUG();

put_out:
	dflc_put(parent);
	return ERR_PTR(err);
}

static struct file *_file_open_create(const char *path, unsigned int flags)
{
	struct dflc *dflc;

	dflc = dflc_lookup(path, flags);
	if (IS_ERR(dflc))
		return ERR_CAST(dflc);

	return &dflc->fp;
}

struct file *file_open(const char *pathname)
{
	return _file_open_create(pathname, 0);
}

struct file *file_open_create(const char *pathname, bool create)
{
	return _file_open_create(pathname, create ? D_CREATE : 0);
}

int vfs_mkdir(const char *pathname, mode_t mode)
{
	struct file *dir;

	dir = _file_open_create(pathname, D_DIR | D_CREATE);
	if (IS_ERR(dir))
		return PTR_ERR(dir);

	file_close(dir);

	return 0;
}

int vfs_stat(const char *pathname, struct stat *st)
{
	struct file *fp;
	int err;

	fp = file_open(pathname);
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	if (!fp->fops->stat)
		return -ENOSYS;

	err = fp->fops->stat(fp, st);
	file_close(fp);

	return err;
}

void *vfs_read_file(const char *pathname, size_t *len)
{
	struct file_handle handle = { 0 };
	struct stat st = { 0 };
	ssize_t err;
	void *ret;

	err = vfs_stat(pathname, &st);
	if (err)
		return ERR_PTR(err);

	ret = kmalloc(st.st_size);
	if (!ret)
		return ERR_PTR(-ENOMEM);

	handle.flags.is_kernel = true;
	handle.fp = file_open(pathname);
	if (IS_ERR(handle.fp))
		return handle.fp;

	err = handle.fp->fops->read(&handle, ret, st.st_size);
	if (err < 0) {
		kfree(ret);
		ret = ERR_PTR(err);
		goto close_out;
	}

	if (len)
		*len = err;

close_out:
	file_close(handle.fp);

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

	list_add(&dev.siblings, &root.children);
	err = devfs.fs_ops->open_file(NULL, &dev.fp, "");
	if (err)
		return err;

	list_add(&ird.siblings, &root.children);
	err = initrdfs.fs_ops->open_file(NULL, &ird.fp, "");
	if (err)
		return err;

	return 0;
}

static void lsof(unsigned int lvl, struct dflc *this)
{
	struct dflc *siblings;

	dflc_lock(this);
	pr("%*s%s%s   (%u)\n", lvl * 2, "",
	   this->name, this->fp.is_directory ? "/" : "", this->refs);
	if (this->fp.is_directory)
		list_for_each_entry(siblings, &this->children, siblings)
			lsof(lvl + 1, siblings);
	dflc_unlock(this);
}

void vfs_lsof(void)
{
	lsof(0, &root);
}
