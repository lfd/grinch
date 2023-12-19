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

#include <grinch/const.h>

#define LOADER_BASE	_UL(0x40000000)
#define VMGRINCH_BASE	_UL(0xffffffc000000000)

/* Check if this applies for ARM64 */
#define USER_START	_UL(0x1000)
#define USER_END	(_UL(1) << 38)
#define USER_STACK_BASE	_UL(0x40000000)
#define USER_STACK_SIZE	_UL(0x2000)

/* Must be a multiple of 256 KiB */
#define GRINCH_SIZE	(2 * 256 * KIB)

#define VMGRINCH_END	(VMGRINCH_BASE + GRINCH_SIZE)

#define IOREMAP_BASE	((VMGRINCH_END + MEGA_PAGE_SIZE - 1) & MEGA_PAGE_MASK)
#define IOREMAP_END	(IOREMAP_BASE + 128 * MIB)
#define IOREMAP_SIZE	(IOREMAP_END - IOREMAP_BASE)

#define KHEAP_BASE      (IOREMAP_END + MEGA_PAGE_SIZE)
#define KHEAP_SIZE      (1 * MIB)
#define KHEAP_END	(KHEAP_BASE + KHEAP_SIZE)

#define DIR_PHYS_BASE	(VMGRINCH_BASE + GIGA_PAGE_SIZE)

#define PERCPU_BASE	(VMGRINCH_BASE + 64UL * GIGA_PAGE_SIZE)
