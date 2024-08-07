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

#define LOADER_BASE	_UL(0x40000000)
#define VMGRINCH_BASE	_UL(0xffffffc000000000)

/* Check if this applies for ARM64 */
#define USER_START		_UL(0x1000)
#define USER_END		(_UL(1) << 38)

#define USER_STACK_SIZE		(1 * MIB)
#define USER_STACK_TOP		USER_END
#define USER_STACK_BOTTOM	(USER_STACK_TOP - USER_STACK_SIZE)

/* Must be a multiple of 256 KiB */
#define GRINCH_SIZE	(8 * 256 * KIB)

#define VMGRINCH_END	(VMGRINCH_BASE + GRINCH_SIZE)

#define IOREMAP_BASE	(VMGRINCH_BASE + 256 * MIB)
#define IOREMAP_SIZE	(512 * MIB)
#define IOREMAP_END	(IOREMAP_BASE + IOREMAP_SIZE)

#define KHEAP_BASE      (IOREMAP_END)

#define DIR_PHYS_BASE	(VMGRINCH_BASE + GIGA_PAGE_SIZE)

#define PERCPU_BASE	(VMGRINCH_BASE + 64UL * GIGA_PAGE_SIZE)
