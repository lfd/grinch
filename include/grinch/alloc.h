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

#ifndef _ALLOC_H
#define _ALLOC_H

#include <grinch/compiler_attributes.h>
#include <grinch/string.h>

int kheap_init(void);
void kheap_stats(void);
size_t kheap_size(void);

void *kmalloc(size_t size);
void kfree(const void *p);

static __always_inline void *kzalloc(size_t size)
{
	void *ret;

	ret = kmalloc(size);
	if (!ret)
		return ret;

	memset(ret, 0, size);

	return ret;
}

#endif /* _ALLOC_H */
