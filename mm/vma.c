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

#define dbg_fmt(x)	"vma: " x

#include <asm_generic/grinch_layout.h>

#include <grinch/errno.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/pmm.h>
#include <grinch/vma.h>

int vma_create(struct vma *vma)
{
	paddr_t phys;
	int err;

	if (vma->flags & VMA_FLAG_LAZY)
		return -ENOSYS;

	err = pmm_page_alloc_aligned(&phys, PAGES(vma->size), PAGE_SIZE, 0);
	if (err)
		return err;

	err = map_range(this_per_cpu()->root_table_page, vma->base, phys,
			vma->size, GRINCH_MEM_RW);
	if (err)
		goto free_out;

	if (vma->flags & VMA_FLAG_ZERO)
		memset(vma->base, 0, vma->size);

	return 0;

free_out:
	pmm_page_free(phys, PAGES(vma->size));
	return err;
}
