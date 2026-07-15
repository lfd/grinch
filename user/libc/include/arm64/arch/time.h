/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2025
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ARCH_TIME_H
#define _ARCH_TIME_H

#include <_internal.h>

static inline int arch_clock_gettime(clockid_t clockid, struct timespec *ts)
{
	return syscall(SYS_clock_gettime, clockid, ts);
}

#endif /* _ARCH_TIME_H */
