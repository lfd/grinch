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
#include <arch/time.h>

int clock_gettime(clockid_t clockid, struct timespec *ts)
{
	return arch_clock_gettime(clockid, ts);
}
