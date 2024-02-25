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
#include <grinch/fs.h>
#include <grinch/syscall.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

static struct file_handle *get_handle(int fd)
{
	struct file_handle *handle;

	if (fd >= MAX_FDS)
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
	long ret;
	int err;

	ret = ustrncpy(path, _path, sizeof(path));
	/* pathname too long */
	if (unlikely(ret == sizeof(path)))
		return -ERANGE;
	else if (unlikely(ret < 0))
		return ret;

	err = check_path(path);
	if (err)
		return err;

	flags = get_flags(oflag);

	task = current_task();
	spin_lock(&task->lock);
	process = task->process;
	for (d = 0; d < MAX_FDS; d++) {
		if (d == 1) continue; // KEEP STDOUT FREE AT THE MOMENT!!
		if (process->fds[d].fp == NULL)
			goto found;
	}

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

	return file->fops->read(handle, buf, count);
}

static int stdout_hack(const char __user *buf, size_t count)
{
#define BLEN	63
	char tmp[BLEN + 1];
	unsigned long sz;
	int err;

	while (count) {
		sz = count < BLEN ? count : BLEN;
		err = copy_from_user(&current_process()->mm, tmp, buf, sz);
		if (err < 0)
			return err;
		tmp[sz] = 0;
		_puts(tmp);

		count -= err;
		buf += err;
	}

	return 0;
}

unsigned long sys_write(int fd, const char __user *buf, size_t count)
{
	struct file_handle *handle;
	struct file *file;

	if (fd == 1)
		return stdout_hack(buf, count);

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

int sys_execve(const char __user *pathname, char *const __user argv[],
	       char *const __user envp[])
{
	struct task *this;
	char buf[MAX_PATHLEN];
	long ret;

	this = current_task();
	ret = ustrncpy(buf, pathname, sizeof(buf));
	/* pathname too long */
	if (unlikely(ret == sizeof(buf)))
		return -ERANGE;
	else if (unlikely(ret < 0))
		return ret;

	uvmas_destroy(this->process);
	this_per_cpu()->pt_needs_update = true;

	return process_from_fs(this, buf);
}
