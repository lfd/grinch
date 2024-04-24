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

#ifndef _VMA_H
#define _VMA_H

#include <grinch/list.h>

#define VMA_FLAG_LAZY	(1 << 0)
#define VMA_FLAG_USER	(1 << 1)
#define VMA_FLAG_R	(1 << 2)
#define VMA_FLAG_W	(1 << 3)
#define VMA_FLAG_X	(1 << 4)
#define VMA_FLAG_RW	(VMA_FLAG_R | VMA_FLAG_W)

struct vma {
	char *name;
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

struct process;

int kvma_create(struct vma *vma);
struct vma *uvma_create(struct task *task, void *base, size_t size,
			unsigned int vma_flags, const char *name);
void uvmas_destroy(struct process *task);

int uvma_duplicate(struct task *t, struct task *src, struct vma *vma);

struct vma *uvma_find(struct process *p, void __user *addr);
bool uvma_collides(struct process *p, void __user *base, size_t size);

int uvma_handle_fault(struct task *t, struct vma *vma, void __user *addr);

#endif /* _VMA_H */
