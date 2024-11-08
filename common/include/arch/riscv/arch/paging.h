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

#ifndef _ARCH_PAGING_H
#define _ARCH_PAGING_H

#if ARCH_RISCV == 32
#define VPN_SHIFT		10
#elif ARCH_RISCV == 64
#define VPN_SHIFT		9
#endif

#define VPN_MASK		((1UL << VPN_SHIFT) - 1)

#define PAGE_SHIFT		12
#define MEGA_PAGE_SHIFT 	(PAGE_SHIFT + VPN_SHIFT)

#endif /* _ARCH_PAGING_H */
