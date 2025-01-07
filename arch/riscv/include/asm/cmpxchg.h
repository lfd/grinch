/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2014 Regents of the University of California
 */

/* Copied from the Linux Kernel sources */

#include <grinch/bits.h>
#include <grinch/const.h>
#include <grinch/panic.h>
#include <grinch/types.h>

#define __arch_cmpxchg_masked(sc_sfx, prepend, append, r, p, o, n)	\
({									\
	u32 *__ptr32b = (u32 *)((ulong)(p) & ~0x3);			\
	ulong __s = ((ulong)(p) & (0x4 - sizeof(*p))) * BITS_PER_BYTE;	\
	ulong __mask = GENMASK(((sizeof(*p)) * BITS_PER_BYTE) - 1, 0)	\
			<< __s;						\
	ulong __newx = (ulong)(n) << __s;				\
	ulong __oldx = (ulong)(o) << __s;				\
	ulong __retx;							\
	ulong __rc;							\
									\
	__asm__ __volatile__ (						\
		prepend							\
		"0:	lr.w %0, %2\n"					\
		"	and  %1, %0, %z5\n"				\
		"	bne  %1, %z3, 1f\n"				\
		"	and  %1, %0, %z6\n"				\
		"	or   %1, %1, %z4\n"				\
		"	sc.w" sc_sfx " %1, %1, %2\n"			\
		"	bnez %1, 0b\n"					\
		append							\
		"1:\n"							\
		: "=&r" (__retx), "=&r" (__rc), "+A" (*(__ptr32b))	\
		: "rJ" ((long)__oldx), "rJ" (__newx),			\
		  "rJ" (__mask), "rJ" (~__mask)				\
		: "memory");						\
									\
	r = (__typeof__(*(p)))((__retx & __mask) >> __s);		\
})

#define __arch_cmpxchg(lr_sfx, sc_sfx, prepend, append, r, p, co, o, n)	\
({									\
	register unsigned int __rc;					\
									\
	__asm__ __volatile__ (						\
		prepend							\
		"0:	lr" lr_sfx " %0, %2\n"				\
		"	bne  %0, %z3, 1f\n"				\
		"	sc" sc_sfx " %1, %z4, %2\n"			\
		"	bnez %1, 0b\n"					\
		append							\
		"1:\n"							\
		: "=&r" (r), "=&r" (__rc), "+A" (*(p))			\
		: "rJ" (co o), "rJ" (n)					\
		: "memory");						\
})

#define _arch_cmpxchg(ptr, old, new, sc_sfx, prepend, append)		\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(*(__ptr)) __old = (old);				\
	__typeof__(*(__ptr)) __new = (new);				\
	__typeof__(*(__ptr)) __ret;					\
									\
	switch (sizeof(*__ptr)) {					\
	case 1:								\
	case 2:								\
		__arch_cmpxchg_masked(sc_sfx, prepend, append,		\
					__ret, __ptr, __old, __new);	\
		break;							\
	case 4:								\
		__arch_cmpxchg(".w", ".w" sc_sfx, prepend, append,	\
				__ret, __ptr, (long), __old, __new);	\
		break;							\
	case 8:								\
		__arch_cmpxchg(".d", ".d" sc_sfx, prepend, append,	\
				__ret, __ptr, /**/, __old, __new);	\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	(__typeof__(*(__ptr)))__ret;					\
})

#define arch_cmpxchg_relaxed(ptr, o, n)					\
	_arch_cmpxchg((ptr), (o), (n), "", "", "")
