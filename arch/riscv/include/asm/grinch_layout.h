/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define _PAGE_SIZE	0x1000
#define _MEGA_PAGE_SIZE	0x200000
#define _GIGA_PAGE_SIZE	0x40000000

#ifdef IS_GUEST
#define VMGRINCH_BASE	0x70000000
#define GRINCH_SIZE	_MEGA_PAGE_SIZE
#else
#define VMGRINCH_BASE	0xffffffc000000000
/*
 * Take the uppermost address that the ioremap area will still be located at
 * the same VPN. This gives us 256MiB ioremap area.
 */
#define IOREMAP_BITS	29
#define IOREMAP_BASE	((void*)(VMGRINCH_BASE | (1 << IOREMAP_BITS)))
#define IOREMAP_SIZE	(1 << (IOREMAP_BITS - 1))

#define GRINCH_SIZE	2 * _MEGA_PAGE_SIZE
#endif

#define PERCPU_BASE	(VMGRINCH_BASE + 64UL * _GIGA_PAGE_SIZE)

#define KMEM_BASE	(PERCPU_BASE + 64UL * _GIGA_PAGE_SIZE)
#define KMEM_SIZE	_GIGA_PAGE_SIZE
