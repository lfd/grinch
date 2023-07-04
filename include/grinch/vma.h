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

#ifndef _VMA_H
#define _VMA_H

#include <grinch/list.h>

#define VMA_FLAG_LAZY	(1 << 0)
#define VMA_FLAG_ZERO	(1 << 1)
#define VMA_FLAG_USER	(1 << 2)
#define VMA_FLAG_EXEC	(1 << 3)
#define VMA_FLAG_R	(1 << 4)
#define VMA_FLAG_W	(1 << 5)
#define VMA_FLAG_RW	(VMA_FLAG_R | VMA_FLAG_W)

struct vma {
	void *base;
	size_t size; /* in bytes */
	unsigned int flags;

	struct list_head vmas;
};

struct mm {
	/* Only lower half must be used */
	page_table_t page_table;

	/* list of struct vma */
	struct list_head vmas;
};

struct task;

int kvma_create(struct vma *vma);
struct vma *uvma_create(struct task *task, void* base, size_t size, unsigned int vma_flags);
void uvmas_destroy(struct task *task);

int uvma_duplicate(struct task *dst, struct task *src, struct vma *vma);

#endif /* _VMA_H */
