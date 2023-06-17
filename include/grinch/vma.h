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

#define VMA_FLAG_LAZY	0x1
#define VMA_FLAG_ZERO	0x2

struct vma {
	void *base;
	size_t size; /* in bytes */
	unsigned int flags;
};

int vma_create(struct vma *vma);

#endif /* _VMA_H */
