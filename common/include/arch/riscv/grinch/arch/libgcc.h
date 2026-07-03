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

#ifndef __ARCH_LIBGCC_H
#define __ARCH_LIBGCC_H

#if ARCH_RISCV == 32

#include <grinch/compiler_attributes.h>

unsigned long long __lshrdi3(unsigned long long u, int b);

unsigned long long __weak __lshrdi3(unsigned long long u, int b)
{
	unsigned int lo = (unsigned int)u;
	unsigned int hi = (unsigned int)(u >> 32);

	if (b == 0)
		return u;
	if (b >= 64)
		return 0;
	if (b >= 32)
		return (unsigned long long)(hi >> (b - 32));

	return ((unsigned long long)(hi >> b) << 32) |
	       ((lo >> b) | (hi << (32 - b)));
}

#endif /* ARCH_RISCV == 32 */

#endif /* __ARCH_LIBGCC_H */
