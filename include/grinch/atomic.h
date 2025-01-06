/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2025
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* Copied from the Linux kernel Sources */

#ifndef _ATOMIC_H
#define _ATOMIC_H

typedef struct {
	int counter;
} atomic_t;

#define ATOMIC_INIT(i)	{ (i) }

#include <grinch/arch/atomic.h>
#include <asm/cmpxchg.h>

static __always_inline int
atomic_fetch_sub_release(int i, atomic_t *v)
{
        __atomic_release_fence();
        return arch_atomic_fetch_sub_relaxed(i, v);
}

static __always_inline int
raw_atomic_cmpxchg_relaxed(atomic_t *v, int old, int new)
{
	return arch_cmpxchg_relaxed(&v->counter, old, new);
}

static __always_inline bool
atomic_try_cmpxchg_relaxed(atomic_t *v, int *old, int new)
{
	int r, o = *old;
	r = raw_atomic_cmpxchg_relaxed(v, o, new);
	if (unlikely(r != o))
		*old = r;
	return likely(r == o);
}

static __always_inline int
atomic_fetch_add_relaxed(int i, atomic_t *v)
{
	return arch_atomic_fetch_add_relaxed(i, v);
}

#endif /* _ATOMIC_H */
