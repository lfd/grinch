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
};

int kvma_create(struct vma *vma);

#endif /* _VMA_H */
