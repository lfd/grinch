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

/* Consumes the file reference on success. */
int fd_new(struct file *file, struct fs_flags flags)
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
			goto found;

	ret = -EMFILE;
	goto unlock_out;

found:
	process->fds[d] = file_handle_new(file, flags);
	if (IS_ERR(process->fds[d])) {
		ret = PTR_ERR(process->fds[d]);
		process->fds[d] = NULL;
		goto unlock_out;
	}

	ret = d;

unlock_out:
	spin_unlock(&task->lock);
	return ret;
}
