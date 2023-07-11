/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _MATH64_H
#define _MATH64_H

#include <grinch/types.h>

static inline u64 div_u64(u64 dividend, u32 divisor)
{
	return dividend / divisor;
}

static inline u64 div_u64_u64(u64 dividend, u64 divisor)
{
	return dividend / divisor;
}

#endif /* _MATH64_H */
