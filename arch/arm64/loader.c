/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/init.h>
#include <grinch/types.h>

void __noreturn grinch_start(u64 p_grinch_dst, u64 p_fdt);
void __noreturn __init loader(paddr_t fdt, paddr_t load_addr);

void __noreturn __init loader(paddr_t fdt, paddr_t load_addr)
{
	for (;;);
}
