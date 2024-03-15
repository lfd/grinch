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

#include <sys/wait.h>

#include <errno.h>
#include <syscall.h>

int waitpid(pid_t pid, int *wstatus, int options)
{
	int ret;

	ret = syscall_3(SYS_wait, pid, (unsigned long)wstatus, options);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
