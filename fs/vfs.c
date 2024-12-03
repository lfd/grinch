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
#include <grinch/fs/tmpfs.h>
#include <grinch/fs/util.h>
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

	bool mountpoint;

	const struct file_system *fs;
	struct file fp;

	spinlock_t lock;
	unsigned int refs;
};

static struct dflc root = {
	.name = "",
	.siblings = LIST_HEAD_INIT(root.siblings),
	.children = LIST_HEAD_INIT(root.children),
	.fp = {
		.mode = S_IFDIR,
	},
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

/* decrease refcount of a single entry */
static void _dflc_put(struct dflc *entry)
{
	struct file *fp;

	dflc_lock(entry);

	fp = &entry->fp;
	entry->refs--;
	if (entry->refs == 0) {
		if (fp->fops && fp->fops->close)
			fp->fops->close(fp);

		if (entry != &root) {
			kfree(entry->name);
			list_del(&entry->siblings);
			kfree(entry);
			entry = NULL;
		}
	}

	if (entry)
		dflc_unlock(entry);
}

/* recursively release the entry */
static void dflc_put(struct dflc *entry)
{
	struct dflc *parent;

	if (entry->refs == 0)
		BUG();

	/*
	 * If we release the dflc entry, we will modify the list in the parent.
	 * Hence, we need to take the lock from the parent. As we might might
	 * have a reverse lookup-task on another CPU, we need to grab the
	 * parent's lock first to prevent deadlocks.
	 */
	parent = entry->parent;
	if (parent)
		dflc_lock(parent);

	_dflc_put(entry);

	if (parent) {
		dflc_unlock(parent);
		dflc_put(parent);
	}
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

	if (!parent->fp.fops || !parent->fp.fops->open) {
		err = -ENOSYS;
		goto err_free_out;
	}

	err = parent->fp.fops->open(&parent->fp, &next->fp, next->name);
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
static struct dflc *
dflc_lookup_at(struct file *at, const char *pathname, unsigned int _flags)
{
	struct dflc *parent, *next;
	const char *start, *end;
	unsigned int flags;
	size_t len;
	int err;

	if (pathname[0] == '/') {
		parent = &root;
		dflc_get(parent);
		pathname++;
	} else {
		if (!at)
			return ERR_PTR(-EINVAL);

		file_dup(at);
		parent = dflc_of(at);
	}

	if (pathname[0] == '\0')
		return parent;

	/* Walk through all directories in between */
	start = end = pathname;
	flags = D_DIR;
	do {
		end = strchrnul(start, '/');
		if (*end == '\0')
			flags = _flags;

		len = end - start;
		if (!strncmp(start, ".", len))
			goto cont;

		if (!strncmp(start, "..", len)) {
			next = parent;
			if (next->parent) {
				next = next->parent;
				dflc_lock(next);
				_dflc_put(parent);
				dflc_unlock(next);
				parent = next;
			}
			goto check;
		}

		dflc_lock(parent);
		next = dflc_lookup_next(parent, start, len, flags);
		dflc_unlock(parent);
		if (IS_ERR(next)) {
			err = PTR_ERR(next);
			goto put_out;
		}

check:
		parent = next;
		if (D_ISDIR(flags) && !S_ISDIR(parent->fp.mode)) {
			err = -ENOTDIR;
			goto put_out;
		}

cont:
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

static struct file *
_file_ocreate_at(struct file *at, const char *path, unsigned int flags)
{
	struct dflc *dflc;

	dflc = dflc_lookup_at(at, path, flags);
	if (IS_ERR(dflc))
		return ERR_CAST(dflc);

	return &dflc->fp;
}

struct file *file_open_at(struct file *at, const char *pathname)
{
	return _file_ocreate_at(at, pathname, 0);
}

struct file *file_ocreate_at(struct file *at, const char *pathname, bool create)
{
	return _file_ocreate_at(at, pathname, create ? D_CREATE : 0);
}

static char *_file_realpath(char *buf, size_t *len, struct dflc *entry)
{
	struct dflc *parent;
	const char *name;
	size_t this_len;

	parent = entry->parent;
	if (parent) {
		buf = _file_realpath(buf, len, parent);
		if (IS_ERR(buf))
			return buf;
	}

	dflc_lock(entry);
	this_len = strlen(entry->name);
	name = entry->name;

	if (*len < (this_len + 1)) {
		dflc_unlock(entry);
		return ERR_PTR(-ENAMETOOLONG);
	}

	strncpy(buf, name, this_len);
	dflc_unlock(entry);
	buf[this_len] = '/';
	this_len += 1;

	*len -= this_len;

	return buf + this_len;
}

char *file_realpath(struct file *file)
{
	char buf[MAX_PATHLEN];
	struct dflc *this;
	size_t len, pos;

	this = dflc_of(file);
	len = sizeof(buf);
	_file_realpath(buf, &len, this);

	if (len == 0)
		return ERR_PTR(-ENAMETOOLONG);

	pos = sizeof(buf) - len;
	pos = pos == 1 ? pos : pos - 1;
	buf[pos] = '\0';

	return kstrdup(buf);
}

int vfs_mkdir_at(struct file *at, const char *pathname, mode_t mode)
{
	struct file *dir;

	dir = _file_ocreate_at(at, pathname, D_DIR | D_CREATE);
	if (IS_ERR(dir))
		return PTR_ERR(dir);

	file_close(dir);

	return 0;
}

int vfs_stat(struct file *file, struct stat *st)
{
	if (!file->fops)
		return -ENOSYS;

	if (!file->fops->stat)
		return -ENOSYS;

	return file->fops->stat(file, st);
}

void *vfs_read_file(struct file *file, size_t *len)
{
	struct file_handle handle = {
		.flags.is_kernel = true,
		.fp = file,
	};
	struct stat st = { 0 };
	ssize_t err;
	void *ret;

	err = vfs_stat(handle.fp, &st);
	if (err)
		return ERR_PTR(err);

	ret = kmalloc(st.st_size);
	if (!ret)
		return ERR_PTR(-ENOMEM);

	err = handle.fp->fops->read(&handle, ret, st.st_size);
	if (err < 0) {
		kfree(ret);
		return ERR_PTR(err);
	}

	if (len)
		*len = err;

	return ret;
}

static int __init
vfs_mount(const char *mountpoint, const struct file_system *fs)
{
	struct dflc *parent;
	struct file *fp;
	int err;

	parent = dflc_lookup_at(NULL, mountpoint, D_DIR);
	if (IS_ERR(parent))
		return PTR_ERR(parent);

	dflc_lock(parent);
	if (parent->refs != 1) {
		err = -EBUSY;
		goto out;
	}

	if (parent->mountpoint) {
		err = -EEXIST;
		goto out;
	}

	fp = &parent->fp;
	if (fp->fops && fp->fops->close)
		fp->fops->close(fp);
	memset(fp, 0, sizeof(*fp));

	parent->mountpoint = true;
	parent->fs = fs;
	err = fs->fs_ops->mount(fs, fp);

out:
	dflc_unlock(parent);

	if (err)
		dflc_put(parent);

	return err;
}

int __init vfs_init(void)
{
	struct file_system *tmpfs;
	int err;

	err = devfs_init();
	if (err)
		return err;

	/* hook in tmpfs */
	tmpfs = kzalloc(sizeof(*root.fs));
	if (!tmpfs)
		return -ENOMEM;

	err = tmpfs_new(tmpfs);
	if (err) {
		kfree(tmpfs);
		return err;
	}

	err = vfs_mount("/", tmpfs);
	if (err)
		return err;

	/* Create directories */
	err = vfs_mkdir_at(NULL, DEVFS_MOUNTPOINT, 0);
	if (err)
		return err;

	err = vfs_mkdir_at(NULL, "/initrd", 0);
	if (err)
		return err;

	err = vfs_mount("/dev", &devfs);
	if (err)
		return err;

	err = vfs_mount("/initrd", &initrdfs);
	if (err)
		return err;

	return 0;
}

static void lsof(unsigned int lvl, struct dflc *this)
{
	struct dflc *siblings;

	dflc_lock(this);
	pr("%*s%s%s   (%u%s)\n", lvl * 2, "",
	   this->name, S_ISDIR(this->fp.mode) ? "/" : "", this->refs,
	   this->mountpoint ? ", mp": "");
	if (S_ISDIR(this->fp.mode))
		list_for_each_entry(siblings, &this->children, siblings)
			lsof(lvl + 1, siblings);
	dflc_unlock(this);
}

void vfs_lsof(void)
{
	lsof(0, &root);
}
