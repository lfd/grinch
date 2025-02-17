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
#include <grinch/align.h>
#include <grinch/gfp.h>
#include <grinch/percpu.h>
#include <grinch/paging.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

static int vma_alloc_range(page_table_t pt, struct vma *vma, void *base,
			   size_t size, unsigned int alignment)
{
	mem_flags_t flags;
	paddr_t phys;
	int err;

	if (!PTR_PAGE_ALIGNED(base))
		return -EINVAL;

	if (size % PAGE_SIZE)
		return -EINVAL;

	err = phys_pages_alloc(&phys, PAGES(size), alignment);
	if (err)
		return err;

	flags = 0;
	if (vma->flags & VMA_FLAG_R)
		flags |= GRINCH_MEM_R;
	if (vma->flags & VMA_FLAG_W)
		flags |= GRINCH_MEM_W;
	if (vma->flags & VMA_FLAG_USER)
		flags |= GRINCH_MEM_U;
	if (vma->flags & VMA_FLAG_X)
		flags |= GRINCH_MEM_X;
	err = map_range(pt, base, phys, size, flags);
	if (err)
		goto free_out;

	return 0;

free_out:
	phys_free_pages(phys, PAGES(size));
	return err;
}

static int
uvma_dealloc_range(page_table_t pt, struct vma *vma, void *base, size_t size)
{
	paddr_t phys;
	size_t step;
	void *this;
	int err;

	step = (vma->flags & VMA_FLAG_LAZY) ? PAGE_SIZE : vma->size;
	for (this = base; this < base + size; this += step) {
		phys = paging_get_phys(pt, this);
		if (phys == INVALID_PHYS_ADDR)
			continue;

		err = phys_free_pages(phys, PAGES(step));
		if (err)
			return err;
	}

	err = unmap_range(pt, base, size);
	if (err)
		return -EINVAL;

	return 0;
}

static int uvma_dealloc(page_table_t pt, struct vma *vma)
{
	return uvma_dealloc_range(pt, vma, vma->base, vma->size);
}

static void uvma_destroy(page_table_t pt, struct vma *vma)
{
	int err;

	kfree(vma->name);
	vma->name = NULL;

	err = uvma_dealloc(pt, vma);
	if (err)
		BUG();
}

static inline int
vma_alloc(page_table_t pt, struct vma *vma, unsigned int alignment)
{
	return vma_alloc_range(pt, vma, vma->base, vma->size, alignment);
}

int kvma_create(struct vma *vma)
{
	int err;

	// FIXME: We must somewhen take care, that this applies to all page
	// tables of all CPUs
	// FIXME: Get alignment via argument
	err = vma_alloc(this_per_cpu()->root_table_page, vma, PAGE_SIZE);
	if (err)
		return err;

	return 0;
}

static struct vma *
__uvma_at(const struct process *p, const void __user *base, size_t size)
{
	struct vma *vma;

	/* Overflow and sanity check */
	if (base + size < base || !size)
		BUG();

	list_for_each_entry(vma, &p->mm.vmas, vmas)
		if (base + size > vma->base && base < vma->base + vma->size)
			return vma;

	return NULL;
}

struct vma *uvma_at(const struct process *p, const void __user *base)
{
	return __uvma_at(p, base, 1);
}

bool uvma_collides(const struct process *p, const void __user *base, size_t size)
{
	return __uvma_at(p, base, size) ? true : false;
}

void uvmas_destroy(struct process *p)
{
	struct list_head *pos, *q;
	struct vma *tmp;

	list_for_each_safe(pos, q, &p->mm.vmas) {
		tmp = list_entry(pos, struct vma, vmas);
		uvma_destroy(p->mm.page_table, tmp);
		list_del(pos);
		kfree(tmp);
	}
}

struct vma *uvma_create(struct task *t, void *base, size_t size,
		        unsigned int vma_flags, const char *name)
{
	struct vma *vma;
	int err;

	if (!is_urange(base, size))
		return ERR_PTR(-ERANGE);

	/* Check that the VMA won't collide with any other VMA */
	if (uvma_collides(&t->process, base, size))
		return ERR_PTR(-EINVAL);

	vma = kmalloc(sizeof(*vma));
	if (!vma)
		return ERR_PTR(-ENOMEM);

	vma->base = base;
	vma->size = size;
	vma->flags = vma_flags | VMA_FLAG_USER;
	if (name) {
		vma->name = kstrdup(name);
		if (!vma->name) {
			kfree(vma);
			return ERR_PTR(-ENOMEM);
		}
	} else
		vma->name = NULL;

	if (!(vma->flags & VMA_FLAG_LAZY)) {
		err = vma_alloc(t->process.mm.page_table, vma, PAGE_SIZE);
		if (err) {
			kfree(vma);
			return ERR_PTR(err);
		}

		/* All pages that are given to the user must be zeroed */
		umemset(t, vma->base, 0, vma->size);
	}

	list_add(&vma->vmas, &t->process.mm.vmas);

	return vma;
}

int uvma_duplicate(struct task *dst, struct task *src, struct vma *vma)
{
	void *base, __user *psrc;
	struct vma *new;
	int err;

	new = uvma_create(dst, vma->base, vma->size, vma->flags, vma->name);
	if (IS_ERR(new))
		return PTR_ERR(new);

	for (base = vma->base; base < vma->base + vma->size;
	     base += PAGE_SIZE) {
		psrc = user_to_direct(&src->process.mm, base);
		/* Skip non-allocated pages */
		if (!psrc)
			continue;

		if (new->flags & VMA_FLAG_LAZY) {
			err = uvma_handle_fault(dst, new, base);
			if (err)
				return err;
		}

		copy_to_user(dst, base, psrc, PAGE_SIZE);
	}

	return 0;
}

int uvma_handle_fault(struct task *t, struct vma *vma, void __user *addr)
{
	void *base;
	int err;

	if (!(vma->flags & VMA_FLAG_LAZY))
		BUG();

	base = PTR_PAGE_ALIGN_DOWN(addr);
	err = vma_alloc_range(t->process.mm.page_table, vma, base,
			      PAGE_SIZE, PAGE_SIZE);
	if (err)
		return err;

	umemset(t, base, 0, PAGE_SIZE);

	return 0;
}

int uvma_resize(const struct process *p, struct vma *vma, size_t size)
{
	int err;

	/*
	 * Currently, we only support resizing on lazy-allocated VMAs. The
	 * reason is that non-lazy VMAs currently can't fragment, while lazy
	 * allocated VMAs can. If we also should ever support resizing for
	 * non-lazy VMAs, then we will have to address destruction of VMAs
	 * (uvma_dealloc_range) accordingly.
	 */
	if (!(vma->flags & VMA_FLAG_LAZY))
		return -ENOSYS;

	/* No page-unaligned resizes, or resize to zero */
	if (size % PAGE_SIZE || size == 0)
		return -EINVAL;

	/* We have nothing to do */
	if (size == vma->size)
		return 0;

	if (size < vma->size) {
		err = uvma_dealloc_range(p->mm.page_table, vma,
					 vma->base + size, vma->size - size);
		if (err)
			return err;
	} else {
		if (uvma_collides(p, vma->base + vma->size, size - vma->size))
			return -ERANGE;

		/*
		 * In case of lazy VMAs: nothing to do. If we ever support
		 * non-lazy growth, then we will have to call uvma_alloc_range
		 * here.
		 */
	}

	vma->size = size;

	return 0;
}
