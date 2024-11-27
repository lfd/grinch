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

int main(void)
{
	printf("  Grinch Base: 0x%16lx\n", GRINCH_BASE);
	printf("  Grinch Size: %luKiB\n", GRINCH_SIZE / 1024);
	printf("    I/O Remap: 0x%16lx -- 0x%16lx\n", IOREMAP_BASE, IOREMAP_END);
	printf("   kheap Base: 0x%16lx\n", KHEAP_BASE);
	printf("Dir phys Base: 0x%16lx\n", DIR_PHYS_BASE);
	printf(" Per CPU Base: 0x%16lx\n", PERCPU_BASE);
	printf("\n");
	printf("   User Start: 0x%16lx\n", USER_START);
	printf("   User   End: 0x%16lx\n", USER_END);

	return 0;
}
