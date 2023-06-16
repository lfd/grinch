/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"kmm: " x

#include <asm_generic/grinch_layout.h>
#include <asm/spinlock.h>

#include <grinch/bitmap.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/paging.h>
#include <grinch/kmm.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/symbols.h>

#define KMM_PAGES	PAGES(GRINCH_SIZE)
#define KMM_SIZE	(KMM_PAGES * PAGE_SIZE)

#if KMM_PAGES % 64 != 0
#error GRINCH_SIZE must fit into internal bitmap without remainder
#endif

static unsigned long kmm_bitmap[BITMAP_SZ(KMM_PAGES)];
static DEFINE_SPINLOCK(kmm_lock);
static paddr_t kmm_pbase;
static ptrdiff_t kmm_v2p_offset;

/* only used once during initialisation */
void kmm_set_v2p_offset(ptrdiff_t diff);

void kmm_set_v2p_offset(ptrdiff_t off)
{
	kmm_v2p_offset = off;
}

static inline bool is_kmm_vaddr(const void *virt)
{
	if ((u64)virt >= VMGRINCH_BASE &&
	    (u64)virt < VMGRINCH_BASE + GRINCH_SIZE)
		return true;

	return false;
}

static inline bool is_kmm_paddr(paddr_t paddr)
{
	if (paddr >= kmm_pbase && paddr < kmm_pbase + KMM_SIZE)
		return true;

	return false;
}

paddr_t kmm_v2p(const void *virt)
{
	if (is_kmm_vaddr(virt))
		return (u64)virt - kmm_v2p_offset;

	while(1);
}

void *kmm_p2v(paddr_t addr)
{
	if (is_kmm_paddr(addr))
		return (void*)addr + kmm_v2p_offset;

	return ERR_PTR(-ENOENT);
}

/* If from (bit) is not zero, then the area MUST start from start. */
static int kmm_find_and_allocate(size_t pages, unsigned int from,
				 unsigned int alignment, paddr_t *paddr)
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

	start = bitmap_find_next_zero_area(kmm_bitmap, KMM_PAGES, from, pages,
					   alignment);
	if (from && start != from)
		return trace_error(-ENOMEM);

	if (start > KMM_PAGES)
		return trace_error(-ENOMEM);

	/* mark as used, return pointer */
	bitmap_set(kmm_bitmap, start, pages);

	*paddr = kmm_pbase + start * PAGE_SIZE;
	return 0;
}

void *kmm_page_alloc_aligned(unsigned int pages, unsigned int alignment,
			     void *hint, unsigned int flags)
{
	paddr_t paddr;
	void *ret;
	int err;
	unsigned int page_offset;

	if (hint)  {
		if (!is_kmm_vaddr(hint) || (u64)hint & PAGE_OFFS_MASK)
			return ERR_PTR(-EINVAL);

		page_offset = (kmm_v2p(hint) - kmm_pbase) / PAGE_SIZE;
	} else {
		page_offset = 0;
	}

	spin_lock(&kmm_lock);
	err = kmm_find_and_allocate(pages, page_offset, alignment, &paddr);
	if (err)
		return ERR_PTR(err);
	spin_unlock(&kmm_lock);

	ret = kmm_p2v(paddr);

	if (flags & KMM_ZERO)
		memset(ret, 0, pages * PAGE_SIZE);

	return ret;
}

int kmm_page_free(const void *addr, unsigned int pages)
{
	unsigned int start;

	if ((unsigned long)addr & PAGE_OFFS_MASK)
		return trace_error(-EINVAL);

	if (!is_kmm_vaddr(addr))
		return trace_error(-EINVAL);

	start = ((paddr_t)addr - VMGRINCH_BASE) / PAGE_SIZE;
	if (start + pages > KMM_PAGES)
		return trace_error(-ERANGE);

	spin_lock(&kmm_lock);
	bitmap_clear(kmm_bitmap, start, pages);
	spin_unlock(&kmm_lock);

	return 0;

}

int kmm_init(void)
{
	pr("OS pages: %lu\n", num_os_pages());
	pr("Internal page pool pages: %lu\n", internal_page_pool_pages());

	kmm_pbase = kmm_v2p(_load_addr);

	/* mark OS pages as used */
	return kmm_mark_used((void*)VMGRINCH_BASE, num_os_pages());
}
