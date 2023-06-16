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

#ifndef _KMM_H
#define _KMM_H

#include <grinch/errno.h>
#include <grinch/compiler_attributes.h>
#include <grinch/types.h>
#include <grinch/paging.h>

#define KMM_ZERO	0x1

int kmm_init(void);

/* May return ENOMEM and EINVAL -> ERR_PTR */
void *kmm_page_alloc_aligned(unsigned int pages, unsigned int alignment, void *hint, unsigned int flags);

static inline int kmm_mark_used(void *addr, unsigned int pages)
{
	void *ret;

	ret = kmm_page_alloc_aligned(pages, PAGE_SIZE, addr, 0);
	if (IS_ERR(ret))
		return PTR_ERR(ret);

	return 0;
}

/* Can only return ENOMEM -> NULL */
static inline void *_kmm_page_alloc(unsigned int pages, unsigned int flags)
{
	void *ret;

	ret = kmm_page_alloc_aligned(pages, PAGE_SIZE, 0, flags);
	if (IS_ERR(ret))
		return NULL;

	return ret;
}

static inline void *kmm_page_alloc(unsigned int pages)
{
	return _kmm_page_alloc(pages, 0);
}

static inline void *kmm_page_zalloc(unsigned int pages)
{
	return _kmm_page_alloc(pages, KMM_ZERO);
}

int kmm_page_free(const void *addr, unsigned int pages);

void *kmm_p2v(paddr_t phys);
paddr_t kmm_v2p(const void *virt);

#endif /* _KMM_H */
