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

#define PMASK(X)	(~((X) - 1))

/* Applies arm64 and rv64. NOT for rv32. */
#define PAGE_SHIFT		12
#define VPN_SHIFT		9
#define VPN_MASK		((1UL << VPN_SHIFT) - 1)

/* Applies to arm64, rv32 and rv64 */
#define PTES_PER_PT		(PAGE_SIZE / sizeof(unsigned long))
#define PAGE_SIZE		_BITUL(PAGE_SHIFT)
#define PAGE_MASK		PMASK(PAGE_SIZE)
#define PAGE_OFFS_MASK		(~PAGE_MASK)

#define MEGA_PAGE_SHIFT 	(PAGE_SHIFT + VPN_SHIFT)
#define MEGA_PAGE_SIZE		_BITUL(MEGA_PAGE_SHIFT)
#define MEGA_PAGE_MASK		PMASK(MEGA_PAGE_SIZE)
#define MEGA_PAGE_OFFS_MASK	(~MEGA_PAGE_MASK)

#define PAGES(X)		((X) / PAGE_SIZE)
#define MEGA_PAGES(X)		((X) / MEGA_PAGE_SIZE)

#ifndef __ASSEMBLY__
static inline u64 page_up(u64 diff)
{
	return (diff + PAGE_SIZE - 1) & PAGE_MASK;
}

static inline u64 mega_page_up(u64 diff)
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
