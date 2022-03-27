#ifndef _BITMAP_H
#define _BITMAP_H

#include <grinch/types.h>

#define BITMAP_SZ(BITS)	(((BITS) + BITS_PER_LONG - 1) / BITS_PER_LONG)

static inline __attribute__((always_inline)) int
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
