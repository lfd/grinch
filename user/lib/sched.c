/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <errno.h>
#include <sched.h>
#include <syscall.h>

int sched_yield(void)
{
	int err;
	err = syscall_0(SYS_sched_yield);
	if (!err)
		return 0;

	if (err < 0)
		errno = -err;

	return -1;
}
