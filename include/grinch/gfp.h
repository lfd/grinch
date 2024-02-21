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

#ifndef _GFP_H
#define _GFP_H

/*
 * GFP aka. Get Free Pages.
 *
 * An allocator that combines kmm (kernel internal pages) and
 * pmm (physical pages).
 */

#include <string.h>

#include <asm/paging_common.h>

#include <grinch/errno.h>

/* Physical Page allocation */
int phys_pages_alloc_aligned(paddr_t *res, size_t pages, unsigned int alignment);
int phys_mark_used(paddr_t addr, size_t pages);
int phys_free_pages(paddr_t from, unsigned int pages);

/* Virtual Page allocation */
void *alloc_pages_aligned(unsigned int pages, unsigned int alignment);

static inline void *alloc_pages(unsigned int pages)
{
	return alloc_pages_aligned(pages, PAGE_SIZE);
}

static inline void *zalloc_pages_aligned(unsigned int pages, unsigned int alignment)
{
	void *ret;

	ret = alloc_pages_aligned(pages, alignment);
	if (!ret)
		return ret;

	memset(ret, 0, pages * PAGE_SIZE);

	return ret;
}

static inline void *zalloc_pages(unsigned int pages)
{
	return zalloc_pages_aligned(pages, PAGE_SIZE);
}

int free_pages(const void *from, unsigned int pages);

/* Translators */
paddr_t v2p(const void *virt);
void *p2v(paddr_t phys);

/* Initialisation */
int kernel_mem_init(void);
int phys_mem_init_fdt(void);
int phys_mem_init(paddr_t addrp, size_t sizep);

#endif /* _GFP_H */
