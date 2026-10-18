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
