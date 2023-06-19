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

/* Must be a multiple of 256 KiB */
#define GRINCH_SIZE	(256 * KIB)

#define VMGRINCH_END	(VMGRINCH_BASE + GRINCH_SIZE)

/*
 * Take the uppermost address that the ioremap area will still be located at
 * the same VPN. This gives us 256MiB ioremap area.
 */
#define IOREMAP_BITS	29
#define IOREMAP_BASE	((void*)(VMGRINCH_BASE | (1 << IOREMAP_BITS)))
#define IOREMAP_SIZE	(1 << (IOREMAP_BITS - 1))
#define IOREMAP_END	(IOREMAP_BASE + IOREMAP_SIZE)

#define PERCPU_BASE	(VMGRINCH_BASE + 64UL * GIGA_PAGE_SIZE)
