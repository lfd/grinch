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

#include <grinch/errno.h>
#include <grinch/iores.h>

#ifdef CONFIG_MMU

int ioremap_init(void);

/* IO mappers */
void *ioremap(paddr_t paddr, size_t size);

int iounmap(const void *vaddr, size_t size);

#else /* !CONFIG_MMU */

/* Devices are reachable where they are, so there is nothing to remap. */
static inline int ioremap_init(void)
{
	return 0;
}

static inline void *ioremap(paddr_t paddr, size_t size)
{
	return (void *)(uintptr_t)paddr;
}

static inline int iounmap(const void *vaddr, size_t size)
{
	return 0;
}

#endif /* CONFIG_MMU */

static inline void *ioremap_area(struct mmio_area *area)
{
	return ioremap(area->paddr, area->size);
}

static inline int ioremap_res(struct mmio_resource *res)
{
	void *base;

	base = ioremap_area(&res->phys);
	if (IS_ERR(base))
		return PTR_ERR(base);

	res->base = base;

	return 0;
}

static inline int iounmap_res(struct mmio_resource *res)
{
	int err;

	if (!res->base)
		return 0;

	err = iounmap(res->base, res->phys.size);
	if (err)
		return err;

	res->base = NULL;

	return 0;
}
