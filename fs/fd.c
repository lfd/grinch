/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"fd: " x

#include <grinch/errno.h>
#include <grinch/fs/vfs.h>
#include <grinch/percpu.h>
#include <grinch/task.h>

/*
 * A reserved slot: taken, so nothing else grabs the descriptor, but not yet
 * pointing at an open file. A process has one task, so the reservation can
 * never be observed through a syscall of its own process.
 */
#define FD_RESERVED	((struct file_handle *)1)

struct file_handle *fd_get(unsigned int fd)
{
	struct file_handle *handle;

	if (fd >= MAX_FDS)
		return ERR_PTR(-EBADF);

	handle = current_process()->fds[fd];
	if (!handle)
		return ERR_PTR(-EINVAL);

	return handle;
}

int fd_reserve(void)
{
	struct process *process;
	struct task *task;
	unsigned int d;
	int ret;

	task = current_task();
	spin_lock(&task->lock);
	process = &task->process;
	for (d = 0; d < MAX_FDS; d++)
		if (process->fds[d] == NULL)
			break;

	if (d == MAX_FDS) {
		ret = -EMFILE;
	} else {
		process->fds[d] = FD_RESERVED;
		ret = d;
	}
	spin_unlock(&task->lock);

	return ret;
}

void fd_install(unsigned int fd, struct file_handle *handle)
{
	struct task *task;

	task = current_task();
	spin_lock(&task->lock);
	if (task->process.fds[fd] != FD_RESERVED)
		BUG();
	task->process.fds[fd] = handle;
	spin_unlock(&task->lock);
}

void fd_release(unsigned int fd)
{
	struct task *task;

	task = current_task();
	spin_lock(&task->lock);
	if (task->process.fds[fd] != FD_RESERVED)
		BUG();
	task->process.fds[fd] = NULL;
	spin_unlock(&task->lock);
}

/* Consumes the file reference on success. */
int fd_new(struct file *file, struct fs_flags flags)
{
	struct file_handle *handle;
	int d;

	d = fd_reserve();
	if (d < 0)
		return d;

	handle = file_handle_new(file, flags);
	if (IS_ERR(handle)) {
		fd_release(d);
		return PTR_ERR(handle);
	}

	fd_install(d, handle);

	return d;
}
