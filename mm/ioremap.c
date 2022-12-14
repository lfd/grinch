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

#define dbg_fmt(x) "ioremap: " x

#include <grinch/bitmap.h>
#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/grinch_layout.h>
#include <grinch/ioremap.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/mm.h>

#define IOREMAP_PAGES	PAGES(IOREMAP_SIZE)

static unsigned long ioremap_bitmap[BITMAP_SZ(IOREMAP_PAGES)];

void *ioremap(paddr_t paddr, size_t size)
{
	unsigned int start;
	unsigned int pages;
	void *ret;
	int err;

	/* resize to next PAGE_SIZE */
	size = page_up(size);
	pages = PAGES(size);

	/* Don't care about alignment at the moment. */
	start = bitmap_find_next_zero_area(ioremap_bitmap, IOREMAP_PAGES,
					   0, pages, 0);
	if (start > IOREMAP_PAGES)
		return ERR_PTR(-ENOMEM);

	ret = IOREMAP_BASE + start * PAGE_SIZE;

	/* TODO: We could mark pages here dirty&accessed */
	err = map_range(this_root_table_page(), ret, paddr, size, PAGE_FLAGS_RW);
	if (err)
		return ERR_PTR(err);

	flush_tlb_all();

	bitmap_set(ioremap_bitmap, start, pages);

	return ret;
}

int iounmap(const void *vaddr, size_t size)
{
	unsigned int start;
	int err;

	if (vaddr < IOREMAP_BASE ||
	    vaddr > IOREMAP_BASE + IOREMAP_SIZE - PAGE_SIZE)
		return -EINVAL;

	size = page_up(size);

	err = unmap_range(this_root_table_page(), vaddr, size);
	if (err)
		return err;

	start = (vaddr - IOREMAP_BASE) / PAGE_SIZE;
	bitmap_clear(ioremap_bitmap, start, PAGES(size));

	return 0;
}
