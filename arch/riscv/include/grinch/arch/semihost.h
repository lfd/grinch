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
	register unsigned long a0 asm("a0") = op;
	register const void *a1 asm("a1") = arg;

	asm volatile(
		".option push\n"
		".option norvc\n"
		"slli x0, x0, 0x1f\n"
		"ebreak\n"
		"srai x0, x0, 7\n"
		".option pop"
		:
		: "r"(a0), "r"(a1)
		: "memory");
}

#endif /* _ARCH_SEMIHOST_H */
