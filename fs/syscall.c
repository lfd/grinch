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

#include <asm-generic/fcntl.h>

#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/fs/vfs.h>
#include <grinch/fs/util.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/syscall.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

static struct fs_flags get_flags(int oflag)
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

	if (oflag & O_CREAT)
		ret.create = true;

	if (oflag & O_DIRECTORY)
		ret.directory = true;

	return ret;
}

static struct file_handle *get_handle(unsigned int fd)
{
	struct file_handle *handle;

	if (fd >= MAX_FDS)
		return ERR_PTR(-EBADF);

	handle = &current_process()->fds[fd];
	if (!handle->fp)
		return ERR_PTR(-EINVAL);

	return handle;
}

SYSCALL_DEF2(open, const char __user *, _pathname, int, oflag)
{
	struct process *process;
	struct fs_flags flags;
	struct file *file;
	struct task *task;
	char *pathname;
	bool must_dir;
	long ret;
	int d;

	pathname = pathname_from_user(_pathname, &must_dir);
	if (IS_ERR(pathname))
		return PTR_ERR(pathname);

	/*
	 * We either must have a directory, if the pathname ends on a /, oder
	 * O_DIRECTORY is set
	 */
	flags = get_flags(oflag);
	must_dir = must_dir | flags.directory;
	if (flags.create && must_dir) {
		ret = -EINVAL;
		goto free_out;
	}

	task = current_task();
	spin_lock(&task->lock);
	process = &task->process;
	for (d = 0; d < MAX_FDS; d++)
		if (process->fds[d].fp == NULL)
			goto found;

	ret = -ENOENT;
	goto unlock_out;

found:
	file = file_ocreate_at(cwd(), pathname, flags.create);
	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		goto unlock_out;
	}

	if (must_dir && !S_ISDIR(file->mode)) {
		file_close(file);
		ret = -ENOTDIR;
		goto unlock_out;
	}

	process->fds[d].fp = file;
	process->fds[d].flags = flags;
	process->fds[d].position = 0;

	ret = d;

unlock_out:
	spin_unlock(&task->lock);
free_out:
	kfree(pathname);

	return ret;
}

SYSCALL_DEF1(close, unsigned int, fd)
{
	struct file_handle *handle;
	struct task *task;
	long err;

	task = current_task();
	spin_lock(&task->lock);

	handle = get_handle(fd);
	if (IS_ERR(handle)) {
		err = PTR_ERR(handle);
		goto unlock_out;
	}

	file_close(handle->fp);
	handle->fp = NULL;
	err = 0;

unlock_out:
	spin_unlock(&task->lock);
	return err;
}

SYSCALL_DEF3(read, unsigned int, fd, char __user *, buf, size_t, count)
{
	struct file_handle *handle;
	struct file *file;
	ssize_t bread;

	if ((ssize_t)count < 0)
		return -EFBIG;

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

SYSCALL_DEF3(write, unsigned int, fd, const char __user *, buf, size_t, count)
{
	struct file_handle *handle;
	struct file *file;

	if ((ssize_t)count < 0)
		return -EFBIG;

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

SYSCALL_DEF2(stat, const char __user *, _pathname, struct stat __user *, _st)
{
	struct stat st = { 0 };
	unsigned long copied;
	struct file *file;
	char *pathname;
	bool must_dir;
	long err;

	pathname = pathname_from_user(_pathname, &must_dir);
	if (IS_ERR(pathname))
		return PTR_ERR(pathname);

	file = file_open_at(cwd(), pathname);
	if (IS_ERR(file)) {
		err = PTR_ERR(file);
		goto path_out;
	}

	err = vfs_stat(file, &st);
	file_close(file);
	if (err)
		goto path_out;

	if (must_dir && !S_ISDIR(st.st_mode)) {
		err = -ENOTDIR;
		goto path_out;
	}

	copied = copy_to_user(current_task(), _st, &st, sizeof(st));
	err = copied == sizeof(st) ? 0 : -EFAULT;

path_out:
	kfree(pathname);

	return err;
}

SYSCALL_DEF3(getdents, unsigned int, fd, struct grinch_dirent __user *, dents,
	     unsigned int, size)
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

SYSCALL_DEF2(mkdir, const char __user *, _pathname, mode_t, mode)
{
	char *pathname;
	int err;

	pathname = pathname_from_user(_pathname, NULL);
	if (IS_ERR(pathname))
		return PTR_ERR(pathname);

	err = vfs_mkdir_at(cwd(), pathname, mode);
	kfree(pathname);

	return err;
}

SYSCALL_DEF2(getcwd, char __user *,buf, size_t, len)
{
	unsigned long cwd_len, copied;
	struct process *p;
	struct task *t;
	int err;

	t = current_task();
	spin_lock(&t->lock);
	p = &t->process;

	if (!p->cwd.pathname) {
		err = -ENOENT;
		goto unlock_out;
	}

	cwd_len = strlen(p->cwd.pathname);
	if (len < (cwd_len + 1)) {
		err = -ERANGE;
		goto unlock_out;
	}

	cwd_len++;

	copied = copy_to_user(t, buf, p->cwd.pathname, cwd_len);
	if (copied != cwd_len) {
		err = -EFAULT;
		goto unlock_out;
	}

	err = 0;

unlock_out:
	spin_unlock(&t->lock);
	return err;
}

SYSCALL_DEF1(chdir, const char __user *, _pathname)
{
	struct task *t;
	char *pathname;
	int err;

	pathname = pathname_from_user(_pathname, NULL);
	if (IS_ERR(pathname))
		return PTR_ERR(pathname);

	t = current_task();
	spin_lock(&t->lock);

	err = process_setcwd(t, pathname);
	if (err)
		goto out;

	err = 0;

out:
	kfree(pathname);
	spin_unlock(&t->lock);

	return err;
}

SYSCALL_DEF3(ioctl, unsigned int, fd, unsigned long, op, unsigned long, arg)
{
	struct file_handle *handle;
	struct file *file;

	handle = get_handle(fd);
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	file = handle->fp;
	if (!file->fops)
		return -EBADF;

	if (!file->fops->ioctl)
		return -EBADF;

	return file->fops->ioctl(file, op, arg);
}
