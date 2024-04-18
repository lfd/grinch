/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"fs-syscall: " x

#include <grinch/errno.h>
#include <grinch/fs/vfs.h>
#include <grinch/fs/util.h>
#include <grinch/syscall.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

static struct file_handle *get_handle(int fd)
{
	struct file_handle *handle;

	if (fd >= MAX_FDS || fd < 0)
		return ERR_PTR(-EBADF);

	handle = &current_process()->fds[fd];
	if (!handle->fp)
		return ERR_PTR(-EINVAL);

	return handle;
}

unsigned long sys_open(const char *_path, int oflag)
{
	struct process *process;
	char path[MAX_PATHLEN];
	struct fs_flags flags;
	struct file *file;
	struct task *task;
	unsigned int d;
	int err;

	err = pathname_from_user(path, _path);
	if (err)
		return err;

	flags = get_flags(oflag);
	flags.must_directory = pathname_sanitise_dir(path);

	task = current_task();
	spin_lock(&task->lock);
	process = &task->process;
	for (d = 0; d < MAX_FDS; d++)
		if (process->fds[d].fp == NULL)
			goto found;

	d = -ENOENT;
	goto unlock_out;

found:
	file = file_open(path, flags);
	if (IS_ERR(file)) {
		d = PTR_ERR(file);
		goto unlock_out;
	}
	process->fds[d].fp = file;
	process->fds[d].flags = flags;
	process->fds[d].position = 0;

unlock_out:
	spin_unlock(&task->lock);
	return d;
}

unsigned long sys_close(int fd)
{
	struct file_handle *handle;
	struct task *task;
	unsigned long err;

	task = current_task();
	spin_lock(&task->lock);

	handle = get_handle(fd);
	if (IS_ERR(handle)) {
		err = PTR_ERR(handle);
		goto unlock_out;
	}

	file_close(handle);
	err = 0;

unlock_out:
	spin_unlock(&task->lock);
	return err;
}

unsigned long sys_read(int fd, char __user *buf, size_t count)
{
	struct file_handle *handle;
	struct file *file;
	ssize_t bread;

	handle = get_handle(fd);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	if (!handle->flags.may_read)
		return -EBADF;

	file = handle->fp;
	if (!file->fops)
		return -EBADF;

	if (!file->fops->read)
		return -EBADF;

	bread = file->fops->read(handle, buf, count);
	if (bread != -EWOULDBLOCK)
		return bread;

	/* we have a blocking read */
	if (handle->flags.nonblock)
		return 0;

	if (!file->fops->register_reader)
		return -EWOULDBLOCK;

	return file->fops->register_reader(handle, buf, count);
}

unsigned long sys_write(int fd, const char __user *buf, size_t count)
{
	struct file_handle *handle;
	struct file *file;

	handle = get_handle(fd);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	if (!handle->flags.may_write)
		return -EBADF;

	file = handle->fp;
	if (!file->fops)
		return -EBADF;

	if (!file->fops->write)
		return -EBADF;

	return file->fops->write(handle, buf, count);
}

int sys_stat(const char __user *_pathname, struct stat __user *_st)
{
	char pathname[MAX_PATHLEN];
	struct stat st = { 0 };
	unsigned long copied;
	bool must_dir;
	int err;

	err = pathname_from_user(pathname, _pathname);
	if (err)
		return err;

	must_dir = pathname_sanitise_dir(pathname);
	err = vfs_stat(pathname, &st);
	if (err)
		return err;

	if (must_dir && !S_ISDIR(st.st_mode))
		return -ENOTDIR;

	copied = copy_to_user(current_task(), _st, &st, sizeof(st));
	err = copied == sizeof(st) ? 0 : -EFAULT;

	return err;
}

int sys_getdents(int fd, struct grinch_dirent __user *dents, unsigned int size)
{
	struct file_handle *handle;
	struct file *file;

	handle = get_handle(fd);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	file = handle->fp;
	if (!file->fops)
		return -EBADF;

	if (!file->fops->getdents)
		return -EBADF;

	return file->fops->getdents(handle, dents, size);
}
