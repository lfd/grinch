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

#include <asm-generic/paging.h>

#include <grinch/errno.h>
#include <grinch/bitmap.h>
#include <grinch/gfp.h>
#include <grinch/init.h>
#include <grinch/mm.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/symbols.h>

long mm_bitmap_find_and_allocate(struct bitmap *bitmap, unsigned int pages,
				 long from, unsigned int alignment)
{
	unsigned long start;

	if (alignment % PAGE_SIZE)
		return trace_error(-EINVAL);
	alignment = PAGES(alignment) - 1;

	start = bitmap_find_next_zero_area(bitmap->bitmap, bitmap->bit_max,
					   from == -1 ? 0 : from,
					   pages, alignment);

	if (from != -1 && start != (unsigned long)from)
		return -ENOMEM;

	if (start > bitmap->bit_max)
		return -ENOMEM;

	/* mark as used, return pointer */
	bitmap_set(bitmap->bitmap, start, pages);

	return start;
}

int __init paging_init(unsigned long this_cpu)
{
	int err;

	pri("=== Grinch memory layout ===\n");
	pri(" Grinch area: 0x%lx -- 0x%lx\n", GRINCH_BASE, GRINCH_END);
#ifdef CONFIG_MMU
	pri("ioremap area: 0x%lx -- 0x%lx\n", IOREMAP_BASE, IOREMAP_END);
	pri("  kheap area: 0x%lx\n", KHEAP_BASE);
	pri(" direct phys: 0x%lx\n", DIR_PHYS_BASE);
#endif
	pri("=== Grinch memory layout end ===\n");

	err = paging_map_kernel(this_cpu);
	if (err)
		return err;

	this_per_cpu()->cpuid = this_cpu;

	return 0;
}

int paging_discard_init(void)
{
	size_t size;
	int err;

	size = page_up(__init_rw_end - __init_text_start);
	pri("Freeing %lu bytes of init code\n", size);

	err = paging_release_init(__init_text_start, size);
	if (err)
		return err;

	return free_pages(__init_text_start, PAGES(size));
}
