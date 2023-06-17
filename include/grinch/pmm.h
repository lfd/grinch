/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _PMM_H
#define _PMM_H

/* Physical memory areas */
int pmm_init_fdt(void);
int pmm_init(paddr_t addrp, size_t sizep);
int pmm_page_alloc_aligned(paddr_t *res, size_t pages,
			   unsigned int alignment, paddr_t hint);
int pmm_page_free(paddr_t phys, size_t pages);


#endif /* _VMA_H */
