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

static int vma_create(page_table_t pt, struct vma *vma)
{
	mem_flags_t flags;
	paddr_t phys;
	int err;

	if (vma->flags & VMA_FLAG_LAZY)
		return -ENOSYS;

	err = pmm_page_alloc_aligned(&phys, PAGES(vma->size), PAGE_SIZE, 0);
	if (err)
		return err;

	flags = 0;
	if (vma->flags & VMA_FLAG_R)
		flags |= GRINCH_MEM_R;
	if (vma->flags & VMA_FLAG_W)
		flags |= GRINCH_MEM_W;
	if (vma->flags & VMA_FLAG_USER)
		flags |= GRINCH_MEM_U;
	if (vma->flags & VMA_FLAG_EXEC)
		flags |= GRINCH_MEM_X;

	err = map_range(pt, vma->base, phys, vma->size, flags);
	if (err)
		goto free_out;

	return 0;

free_out:
	pmm_page_free(phys, PAGES(vma->size));
	return err;
}

int kvma_create(struct vma *vma)
{
	int err;

	// FIXME: We must somewhen take care, that this applies to all page
	// tables of all CPUs
	err = vma_create(this_per_cpu()->root_table_page, vma);
	if (err)
		return err;

	if (vma->flags & VMA_FLAG_ZERO)
		memset(vma->base, 0, vma->size);

	return 0;
}
