/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define DEFINE(sym, val) \
	asm volatile("\n=>" #sym " %0 " #val : : "i" (val))

#define BLANK() asm volatile("\n=>" : : )

#define OFFSET(sym, str, mem) \
	DEFINE(sym, __builtin_offsetof(struct str, mem))

#define COMMENT(x) \
	asm volatile("\n=>#" x)

void common(void);
