/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _BITMAP_H
#define _BITMAP_H

#include <grinch/align.h>
#include <grinch/alloc.h>
#include <grinch/bitops.h>
#include <grinch/bits.h>
#include <grinch/compiler_attributes.h>
#include <grinch/types.h>

#define bitmap_size(nbits)	(ALIGN(nbits, BITS_PER_LONG) / BITS_PER_BYTE)
#define DECLARE_BITMAP(name, bits)	unsigned long name[BITS_TO_LONGS(bits)]

struct bitmap {
	unsigned long *bitmap;
	unsigned long bit_max;
};

static __always_inline int
test_bit(unsigned int nr, const volatile unsigned long *addr)
{
	return ((1UL << (nr % BITS_PER_LONG)) &
		(addr[nr / BITS_PER_LONG])) != 0;
}

unsigned long bitmap_find_next_zero_area_off(unsigned long *map,
					     unsigned long size,
					     unsigned long start,
					     unsigned int nr,
					     unsigned long align_mask,
					     unsigned long align_offset);

void bitmap_set(unsigned long *map, unsigned int start, unsigned int nbits);
void bitmap_clear(unsigned long *map, unsigned int start, unsigned int nbits);

/* Allocate a zeroed bitmap holding @bits bits, or NULL on failure. */
static inline unsigned long *bitmap_zalloc(unsigned long bits)
{
	return kzalloc(bitmap_size(bits));
}

static inline unsigned long
bitmap_find_next_zero_area(unsigned long *map,
			   unsigned long size,
			   unsigned long start,
			   unsigned int nr,
			   unsigned long align_mask)
{
	return bitmap_find_next_zero_area_off(map, size, start, nr,
					      align_mask, 0);
}

#endif /* _BITMAP_H */
