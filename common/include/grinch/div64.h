/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* Copied and adapted from the Linux Kernel Sources */

#ifndef _MATH64_H
#define _MATH64_H

#include <grinch/types.h>

static inline u64 div_u64(u64 dividend, u32 divisor)
{
	return dividend / divisor;
}


#define do_div(n,base) ({					\
	u32 __base = (base);					\
	u32 __rem;						\
	__rem = ((u64)(n)) % __base;				\
	(n) = ((u64)(n)) / __base;				\
	__rem;							\
 })

#endif /* _MATH64_H */
