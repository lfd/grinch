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

#ifndef _MM_H
#define _MM_H

#include <grinch/bitmap.h>

/* If from (bit) is not zero, then the area MUST start from start. */
long mm_bitmap_find_and_allocate(struct bitmap *bitmap, size_t pages, unsigned
				 int from, unsigned int alignment);

#endif /* _MM_H */
