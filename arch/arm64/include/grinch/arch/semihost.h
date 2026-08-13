/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef _ARCH_SEMIHOST_H
#define _ARCH_SEMIHOST_H

static inline void semihosting_call(unsigned long op, const void *arg)
{
	register unsigned long x0 asm("x0") = op;
	register const void *x1 asm("x1") = arg;

	asm volatile(
		"hlt #0xf000"
		:
		: "r"(x0), "r"(x1)
		: "memory");
}

#endif /* _ARCH_SEMIHOST_H */
