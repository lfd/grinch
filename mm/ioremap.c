/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
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
#include <grinch/bootparam.h>
#include <grinch/cpu.h>
#include <grinch/gfp.h>
#include <grinch/ioremap.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>

#define IOREMAP_PAGES	PAGES(IOREMAP_SIZE)

static unsigned long ioremap_bitmap[BITMAP_ELEMS(IOREMAP_PAGES)];
static size_t ioremap_pages = IOREMAP_PAGES;

static void __init ioremap_size_parse(const char *arg)
{
	size_t sz;
	int err;

	err = bootparam_parse_size(arg, &sz);
	if (err) {
		pri("Warning: Unable to parse ioremap_size=%s\n", arg);
		return;
	}

	sz = page_up(sz);
	if (sz > IOREMAP_SIZE) {
		pri("Warning: ioremap_size=%s exceeds the window\n", arg);
		return;
	}

	ioremap_pages = PAGES(sz);
}
bootparam(ioremap_size, ioremap_size_parse);

/*
 * No kernel mapping may create a new root-level slot after boot:
 * process page tables only receive a copy of the kernel's root entries
 * on activation. The ioremap window is the only kernel region that
 * grows at runtime - populate its root slots up front.
 */
int __init ioremap_init(void)
{
	return paging_prealloc(this_root_table_page(), (void *)IOREMAP_BASE,
			       ioremap_pages * PAGE_SIZE);
}

void __init *ioremap(paddr_t paddr, size_t size)
{
	unsigned int start, pages, paddr_al, size_al;
	unsigned long align_mask;
	void *ret;
	int err;

	/* resize to next PAGE_SIZE */
	size = page_up(size);
	pages = PAGES(size);

	paddr_al = __ffsl(paddr);
	size_al = __ffsl(size);
	if (size_al <= paddr_al)
		align_mask = PAGES(1 << size_al) - 1;
	else
		align_mask = 0;

retry:
	start = bitmap_find_next_zero_area(ioremap_bitmap, ioremap_pages,
					   0, pages, align_mask);
	if (start > ioremap_pages) {
		if (align_mask == 0)
			return ERR_PTR(-ENOMEM);

		align_mask = 0;
		goto retry;
	}

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
	uintptr_t end = IOREMAP_BASE + ioremap_pages * PAGE_SIZE;
	uintptr_t addr = (uintptr_t)vaddr;

	if (addr < IOREMAP_BASE || addr >= end)
		return false;

	if (addr + pages * PAGE_SIZE > end)
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
