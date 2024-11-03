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

#ifndef _PAGING_COMMON_H
#define _PAGING_COMMON_H

#include <grinch/const.h>
#include <grinch/types.h>
#include <arch/paging.h>

#define PMASK(X)	(~((X) - 1))

#define PAGE_SIZE		_BITUL(PAGE_SHIFT)
#define PAGE_MASK		PMASK(PAGE_SIZE)
#define PAGE_OFFS_MASK		(~PAGE_MASK)

#define MEGA_PAGE_SIZE		_BITUL(MEGA_PAGE_SHIFT)
#define MEGA_PAGE_MASK		PMASK(MEGA_PAGE_SIZE)
#define MEGA_PAGE_OFFS_MASK	(~MEGA_PAGE_MASK)

#define PTES_PER_PT		(PAGE_SIZE / sizeof(unsigned long))

#define PAGES(X)		((X) / PAGE_SIZE)
#define MEGA_PAGES(X)		((X) / MEGA_PAGE_SIZE)

#ifndef __ASSEMBLY__

static inline unsigned long page_up(unsigned long diff)
{
	return (diff + PAGE_SIZE - 1) & PAGE_MASK;
}

static inline unsigned long mega_page_up(unsigned long diff)
{
	return (diff + MEGA_PAGE_SIZE - 1) & MEGA_PAGE_MASK;
}

static inline unsigned int page_offset(const paddr_t addr)
{
	return addr & PAGE_OFFS_MASK;
}

static inline unsigned int page_voffset(const void *page)
{
	return (uintptr_t)page & PAGE_OFFS_MASK;
}

static inline unsigned int page_bytes_left(const void *p)
{
	return PAGE_SIZE - page_voffset(p);
}
#endif /* __ASSEMBLY__ */

#endif /* _PAGING_COMMON_H */
