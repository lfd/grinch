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

#include "../common/include/grinch/const.h"
#include "../include/asm-generic/grinch_layout.h"

#include <stdio.h>

#define PMASK(X)	(~((X) - 1))
#define MEGA_PAGE_SIZE	(2 * MIB)
#define MEGA_PAGE_MASK	PMASK(MEGA_PAGE_SIZE)

#define GIGA_PAGE_SIZE	(1 * GIB)

int main(void)
{
	printf("  Grinch Size: %luKiB\n", GRINCH_SIZE / 1024);
	printf("  Loader Base: 0x%16lx\n", LOADER_BASE);
	printf("VMGrinch Base: 0x%16lx\n", VMGRINCH_BASE);
	printf("    I/O Remap: 0x%16lx -- 0x%016lx\n", IOREMAP_BASE, IOREMAP_END);
	printf("   kheap Base: 0x%16lx\n", KHEAP_BASE);
	printf("Dir phys Base: 0x%16lx\n", DIR_PHYS_BASE);
	printf(" Per CPU Base: 0x%16lx\n", PERCPU_BASE);

	return 0;
}
