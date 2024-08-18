/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "ioremap: " x

#include <grinch/bitmap.h>
#include <grinch/bitops.h>
#include <grinch/cpu.h>
#include <grinch/ioremap.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>

#define IOREMAP_PAGES	PAGES(IOREMAP_SIZE)

static unsigned long ioremap_bitmap[BITMAP_ELEMS(IOREMAP_PAGES)];

void __init *ioremap(paddr_t paddr, size_t size)
{
	unsigned int start, pages, paddr_al, size_al;
	unsigned long align_mask;
	void *ret;
	int err;

	/* resize to next PAGE_SIZE */
	size = page_up(size);
	pages = PAGES(size);

	paddr_al = ffsl(paddr);
	size_al = ffsl(size);
	if (size_al <= paddr_al)
		align_mask = PAGES(1 << size_al) - 1;
	else
		align_mask = 0;

	start = bitmap_find_next_zero_area(ioremap_bitmap, IOREMAP_PAGES,
					   0, pages, align_mask);
	if (start > IOREMAP_PAGES)
		return ERR_PTR(-ENOMEM);

	ret = (void *)IOREMAP_BASE + (start * PAGE_SIZE) + page_offset(paddr);
	err = map_range(this_root_table_page(), ret, paddr, size,
			GRINCH_MEM_DEVICE | GRINCH_MEM_RW);
	if (err)
		return ERR_PTR(err);

	/* FIXME: could be more fine-granular */
	flush_tlb_all();

	bitmap_set(ioremap_bitmap, start, pages);

	return ret;
}

static bool __init is_ioremap(const void *vaddr, size_t pages)
{
	uintptr_t addr = (uintptr_t)vaddr;
	if (addr < IOREMAP_BASE || addr >= IOREMAP_END)
		return false;

	if (addr + pages * PAGE_SIZE > IOREMAP_END)
		return false;

	return true;
}

int __init iounmap(const void *vaddr, size_t size)
{
	unsigned int start, pages;
	int err;

	size = page_up(size);
	pages = PAGES(size);
	vaddr = (const void *)((uintptr_t)vaddr & PAGE_MASK);

	if (!is_ioremap(vaddr, pages))
		return -ERANGE;

	err = unmap_range(this_root_table_page(), vaddr, size);
	if (err)
		return err;

	/* FIXME: could be more fine-granular */
	flush_tlb_all();

	start = (vaddr - (void*)IOREMAP_BASE) / PAGE_SIZE;
	bitmap_clear(ioremap_bitmap, start, pages);

	return 0;
}
