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

#ifndef _TIME_H
#define _TIME_H

#include <syscall.h>

/* returns the elapsed time since boot in nsec */
static inline unsigned long gettime(void)
{
	return syscall_0(SYS_gettime);
}

#endif /* _TIME_H */
