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

#ifndef _ASM_COUNTER_H
#define _ASM_COUNTER_H

#include <grinch/types.h>

/*
 * A counter wider than the register reading it: the halves arrive one after
 * the other, so retry until the upper one holds still across the lower.
 */
#define read_counter64(upper, lower)					\
({									\
	u32 __hi, __lo;							\
									\
	do {								\
		__hi = (upper);						\
		__lo = (lower);						\
	} while (__hi != (upper));					\
									\
	((u64)__hi << 32) | __lo;					\
})

#endif /* _ASM_COUNTER_H */
