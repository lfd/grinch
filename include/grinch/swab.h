/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/types.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
static inline u32 __swab32(const u32 x)
{
	const u8 *_x = (u8*)&x;
	union {
		u8 raw[sizeof(u32)];
		u32 ret;
	} ret;

	ret.raw[0] = _x[3];
	ret.raw[1] = _x[2];
	ret.raw[2] = _x[1];
	ret.raw[3] = _x[0];

	return ret.ret;
}

static inline u64 __swab64(const u64 x)
{
	const u32 *_x = (u32*)&x;
	u64 ret;

	ret = ((u64)__swab32(_x[0]) << 32) | __swab32(_x[1]);

	return ret;
}
#endif

static inline unsigned long swab(unsigned long val)
{
#if BITS_PER_LONG == 64
	return __swab64(val);
#else /* 32 */
	return __swab32(val);
#endif
}
