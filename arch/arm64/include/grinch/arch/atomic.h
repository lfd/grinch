/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ARCH_ATOMIC_H
#define _ARCH_ATOMIC_H

#include <asm-generic/rwonce.h>
#include <asm/cpu.h>
#include <asm/atomic_ll_sc.h>

static __always_inline int atomic_read(const atomic_t *v)
{
	return READ_ONCE(v->counter);
}

static __always_inline void atomic_set(atomic_t *v, int i)
{
	WRITE_ONCE(v->counter, i);
}

#define __atomic_release_fence()	dmb(ish)

#define arch_atomic_fetch_add_relaxed(i, v) \
	__ll_sc_atomic_fetch_add_relaxed(i, v)

#define arch_atomic_fetch_sub_relaxed(i, v) \
	__ll_sc_atomic_fetch_sub_relaxed(i, v)

#endif /* _ARCH_ATOMIC_H */
