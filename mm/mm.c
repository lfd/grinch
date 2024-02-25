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
#include <grinch/printk.h>
#include <grinch/mm.h>

long mm_bitmap_find_and_allocate(struct bitmap *bitmap, size_t pages, unsigned
				 int from, unsigned int alignment)
{
	unsigned int start;

	if (alignment % PAGE_SIZE)
		return trace_error(-EINVAL);
	alignment = PAGES(alignment) - 1;

	start = bitmap_find_next_zero_area(bitmap->bitmap, bitmap->bit_max,
					   from, pages, alignment);
	if (from && start != from)
		return -ENOMEM;

	if (start > bitmap->bit_max)
		return -ENOMEM;

	/* mark as used, return pointer */
	bitmap_set(bitmap->bitmap, start, pages);

	return start;
}
