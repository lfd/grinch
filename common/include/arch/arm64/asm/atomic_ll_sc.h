/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/atomic.h
 *
 * Copyright (C) 1996 Russell King.
 * Copyright (C) 2002 Deep Blue Solutions Ltd.
 * Copyright (C) 2012 ARM Ltd.
 */

/* Derived from Linux arch/arm64/include/asm/atomic_ll_sc.h */

#ifndef __ASM_ATOMIC_LL_SC_H
#define __ASM_ATOMIC_LL_SC_H

#include <grinch/stringify.h>

/*
 * Relaxed fetch-and-op using LL/SC (ldxr/stxr).  Works on all ARMv8-A CPUs.
 */
#define ATOMIC_FETCH_OP_RELAXED(op, asm_op, constraint)			\
static __always_inline int						\
__ll_sc_atomic_fetch_##op##_relaxed(int i, atomic_t *v)			\
{									\
	unsigned long tmp;						\
	int val, result;						\
									\
	asm volatile(							\
	"1:	ldxr	%w[result], %[v]\n"				\
	"	" #asm_op "	%w[val], %w[result], %w[i]\n"		\
	"	stxr	%w[tmp], %w[val], %[v]\n"			\
	"	cbnz	%w[tmp], 1b"					\
	: [result] "=&r" (result),					\
	  [val]    "=&r" (val),						\
	  [tmp]    "=&r" (tmp),						\
	  [v]      "+Q"  (v->counter)					\
	: [i]      __stringify(constraint) "r" (i)			\
	);								\
	return result;							\
}

ATOMIC_FETCH_OP_RELAXED(add, add, I)
ATOMIC_FETCH_OP_RELAXED(sub, sub, J)

#undef ATOMIC_FETCH_OP_RELAXED

/* Relaxed 32-bit cmpxchg using LL/SC.  Returns the old value. */
static __always_inline int
__ll_sc_cmpxchg_relaxed(volatile int *ptr, int old, int new)
{
	unsigned long tmp;
	int oldval;

	asm volatile(
	"1:	ldxr	%w[oldval], %[v]\n"
	"	eor	%w[tmp], %w[oldval], %w[old]\n"
	"	cbnz	%w[tmp], 2f\n"
	"	stxr	%w[tmp], %w[new], %[v]\n"
	"	cbnz	%w[tmp], 1b\n"
	"2:"
	: [oldval] "=&r" (oldval),
	  [tmp]    "=&r" (tmp),
	  [v]      "+Q"  (*ptr)
	: [old]    "r"   (old),
	  [new]    "r"   (new)
	);
	return oldval;
}

#endif /* __ASM_ATOMIC_LL_SC_H */
