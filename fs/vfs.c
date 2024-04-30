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
static LIST_HEAD(mountpoints);

struct mountpoint {
	struct {
		char *name;
		unsigned int len;
	} path;
	const struct file_system *fs;

	struct list_head mountpoints;
};

struct files {
	struct list_head files;

	const char *pathname;
	bool is_directory;
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

		if (flags.must_directory && !files->is_directory)
			return ERR_PTR(-ENOTDIR);

		files->refs++;
		return &files->fp;
	}

	return ERR_PTR(-ENOENT);
}

static const struct mountpoint *
find_mountpoint(const char *pathname, const char **_fsname)
{
	struct mountpoint *mp, *cand;
	const char *fsname;
	char successor;

	mp = NULL;
	list_for_each_entry(cand, &mountpoints, mountpoints) {
		if (strncmp(cand->path.name, pathname, cand->path.len))
			continue;

		successor = pathname[cand->path.len];
		if (successor != '\0' && successor != '/')
			continue;

		if (!mp) {
			mp = cand;
			continue;
		}

		if (cand->path.len > mp->path.len)
			mp = cand;
	}

	if (!mp)
		return NULL;

	if (_fsname) {
		fsname = pathname + mp->path.len;
		if (*fsname == '/')
			fsname++;
		*_fsname = fsname;
	}

	return mp;
}

/* must hold files_lock */
static struct file *_file_open(const char *pathname, struct fs_flags flags)
{
	const struct file_system *fs;
	const struct mountpoint *mp;
	struct files *files;
	const char *fsname;
	int err;

	mp = find_mountpoint(pathname, &fsname);
	if (!mp)
		return ERR_PTR(-ENOENT);

	fs = mp->fs;

	files = kzalloc(sizeof(*files));
	if (!files)
		return ERR_PTR(-ENOMEM);

	files->pathname = pathname;
	err = fs->fs_ops->open_file(fs, &files->fp, fsname, flags);
	if (err) {
		kfree(files);
		return ERR_PTR(err);
	}

	files->refs = 1;
	list_add(&files->files, &open_files);

	return &files->fp;
}

struct file *file_open(const char *_path, struct fs_flags flags)
{
	struct file *filep;
	char *path;

	path = kstrdup(_path);
	if (!path)
		return ERR_PTR(-ENOMEM);

	spin_lock(&files_lock);
	filep = search_file(path, flags);
	if (filep != ERR_PTR(-ENOENT)) {
		kfree(path);
		goto unlock_out;
	}

	filep = _file_open(path, flags);
	if (IS_ERR(filep))
		kfree(path);

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

struct fs_flags get_flags(int oflag)
{
	struct fs_flags ret = { 0 };

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

int vfs_stat(const char *pathname, struct stat *st)
{
	const struct mountpoint *mp;
	const char *fsname;

	mp = find_mountpoint(pathname, &fsname);
	if (!mp)
		return -ENOENT;

	if (!mp->fs->fs_ops)
		return -ENOSYS;

	if (!mp->fs->fs_ops->stat)
		return -ENOSYS;

	return mp->fs->fs_ops->stat(mp->fs, fsname, st);
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
	handle.fp = file_open(pathname, handle.flags);
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
	file_close(&handle);

	return ret;
}

static int __init vfs_mount(const char *path, const struct file_system *fs)
{
	struct mountpoint *mp;
	int err;

	mp = kzalloc(sizeof(*mp));
	if (!mp)
		return -ENOMEM;

	mp->path.name = kstrdup(path);
	if (!mp->path.name) {
		err = -ENOMEM;
		goto free_out;
	}
	mp->path.len = strlen(mp->path.name);
	mp->fs = fs;

	list_add(&mp->mountpoints, &mountpoints);
	return 0;

free_out:
	kfree(mp);
	return err;
}

int __init vfs_init(void)
{
	int err;

	err = initrd_init();
	if (err == -ENOENT)
		pri("No ramdisk found\n");
	else if (err)
		return err;

	err = vfs_mount("/initrd", &initrdfs);
	if (err)
		return err;

	err = devfs_init();
	if (err)
		return err;

	err = vfs_mount(DEVFS_MOUNTPOINT, &devfs);
	if (err)
		return err;

	return 0;
}
