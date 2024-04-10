/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"vma: " x

#include <grinch/alloc.h>
#include <grinch/gfp.h>
#include <grinch/percpu.h>
#include <grinch/paging.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

static int vma_create(page_table_t pt, struct vma *vma, unsigned int alignment)
{
	mem_flags_t flags;
	paddr_t phys;
	int err;

	if (vma->flags & VMA_FLAG_LAZY)
		return -ENOSYS;

	err = phys_pages_alloc_aligned(&phys, PAGES(vma->size), alignment);
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
	phys_free_pages(phys, PAGES(vma->size));
	return err;
}

int kvma_create(struct vma *vma)
{
	int err;

	// FIXME: We must somewhen take care, that this applies to all page
	// tables of all CPUs
	// FIXME: Get alignment via argument
	err = vma_create(this_per_cpu()->root_table_page, vma, MEGA_PAGE_SIZE);
	if (err)
		return err;

	return 0;
}

static bool is_user_range(void *_base, size_t size)
{
	u64 base = (u64)_base;

	if (base >= USER_START && base < USER_END &&
	    (base + size) <= USER_END)
		return true;

	return false;
}

static bool vma_collides(struct vma *vma, void *base, size_t size)
{
	if (vma->base < base && vma->base + vma->size > base)
		return true;

	if (base < vma->base && base + size > vma->base)
		return true;

	return false;
}

static int uvma_destroy(struct process *p, struct vma *vma)
{
	int err;
	paddr_t phys;

	/* Doesn't understand lazy VMAs! */
	phys = paging_get_phys(p->mm.page_table, vma->base);
	if (phys == INVALID_PHYS_ADDR)
		return -EINVAL;

	err = unmap_range(p->mm.page_table, vma->base, vma->size);
	if (err)
		return -EINVAL;

	err = phys_free_pages(phys, PAGES(vma->size));
	if (err)
		return -EINVAL;

	return 0;
}

void uvmas_destroy(struct process *p)
{
	struct list_head *pos, *q;
	struct vma *tmp;
	int err;

	list_for_each_safe(pos, q, &p->mm.vmas) {
		tmp = list_entry(pos, struct vma, vmas);
		err = uvma_destroy(p, tmp);
		if (err)
			BUG();
		list_del(pos);
		kfree(tmp);
	}
}

struct vma *uvma_create(struct process *p, void *base, size_t size, unsigned int vma_flags)
{
	struct vma *vma;
	int err;

	if (!is_user_range(base, size))
		return ERR_PTR(-ERANGE);

	/* Check that the VMA won't collide with any other VMA */
	list_for_each_entry(vma, &p->mm.vmas, vmas)
		if (vma_collides(vma, base, size))
			return ERR_PTR(-EINVAL);

	vma = kmalloc(sizeof(*vma));
	if (!vma)
		return ERR_PTR(-ENOMEM);

	vma->base = base;
	vma->size = size;
	vma->flags = vma_flags | VMA_FLAG_USER;

	err = vma_create(p->mm.page_table, vma, PAGE_SIZE);
	if (err) {
		kfree(vma);
		return ERR_PTR(err);
	}

	list_add(&vma->vmas, &p->mm.vmas);

	/* All pages that are given to the user must be zeroed */
	umemset(&p->mm, vma->base, 0, vma->size);

	return vma;
}

int uvma_duplicate(struct process *dst, struct process *src, struct vma *vma)
{
	void *psrc, *pdst;
	struct vma *new;

	new = uvma_create(dst, vma->base, vma->size, vma->flags);
	if (IS_ERR(new))
		return PTR_ERR(new);

	psrc = user_to_direct(&src->mm, vma->base);
	if (!psrc)
		return -EINVAL;

	pdst = user_to_direct(&dst->mm, new->base);
	if (!pdst)
		return -EINVAL;

	memcpy(pdst, psrc, vma->size);

	return 0;
}
