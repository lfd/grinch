/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

extern unsigned char __vmgrinch_bin_start[];
extern unsigned char __vmgrinch_bin_size[];

#define vmgrinch_size	((size_t)__vmgrinch_bin_size)

static inline void loader_copy_grinch(void)
{
	/* Copy grinch to destination */
	memcpy((void*)VMGRINCH_BASE, __vmgrinch_bin_start, vmgrinch_size);

	/*
	 * Kernel memory shall be all zeroed. We don't know what is initially
	 * inside per_cpu structures, and they won't be initialised by zeroes.
	 */
	memset((void*)VMGRINCH_BASE + vmgrinch_size, 0,
	       ((VMGRINCH_END - VMGRINCH_BASE) - vmgrinch_size));


}
