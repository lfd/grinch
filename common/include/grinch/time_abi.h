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

#ifndef _GRINCH_TIME_ABI_H
#define _GRINCH_TIME_ABI_H

#include <grinch/types.h>

typedef s64 time_t;
typedef u64 timeu_t;

typedef int clockid_t;

struct timespec {
	time_t tv_sec;
	time_t tv_nsec;
};

#endif /* _GRINCH_TIME_ABI_H */
