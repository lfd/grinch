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

#include <grinch/iores.h>

/* IO mappers */
void *ioremap(paddr_t paddr, size_t size);

static inline void *ioremap_area(struct mmio_area *area)
{
	return ioremap(area->paddr, area->size);
}

int iounmap(const void *vaddr, size_t size);
