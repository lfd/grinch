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

#include <syscall.h>
#include <time.h>

int clock_gettime(clockid_t clockid, struct timespec *ts)
{
	return errno_syscall_2(SYS_clock_gettime, clockid, (uintptr_t)ts);
}
