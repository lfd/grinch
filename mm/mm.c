/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/errno.h>
#include <grinch/bitmap.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/mm.h>

long mm_bitmap_find_and_allocate(struct bitmap *bitmap, size_t pages, unsigned
				 int from, unsigned int alignment)
{
	unsigned int start;

	switch (alignment) {
		case PAGE_SIZE:
			alignment = 0;
			break;
		case MEGA_PAGE_SIZE:
			alignment = PAGES(MEGA_PAGE_SIZE) - 1;
			break;
		case GIGA_PAGE_SIZE:
			alignment = PAGES(GIGA_PAGE_SIZE) - 1;
			break;
		default:
			return trace_error(-EINVAL);
	};

	start = bitmap_find_next_zero_area(bitmap->bitmap, bitmap->bit_max,
					   from, pages, alignment);
	if (from && start != from)
		return trace_error(-ENOMEM);

	if (start > bitmap->bit_max)
		return trace_error(-ENOMEM);

	/* mark as used, return pointer */
	bitmap_set(bitmap->bitmap, start, pages);

	return start;
}
