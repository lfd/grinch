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

#ifndef _GRINCH_IORES_H
#define _GRINCH_IORES_H

typedef unsigned char iores_flags_t;

#define IORES_MEM	(1 << 0)
#define IORES_MEM_64	(1 << 1)
#define IORES_PREFETCH	(1 << 2)

struct mmio_area {
	paddr_t paddr;
	size_t size;
};

struct mmio_resource {
	struct mmio_area phys;
	void *base;
};

#endif /* _GRINCH_IORES_H */
