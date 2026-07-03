/*
 * Jailhouse AArch64 support
 *
 * Copyright (C) 2015 Huawei Technologies Duesseldorf GmbH
 *
 * Authors:
 *  Antonios Motakis <antonios.motakis@huawei.com>
 *
 * Spinlock implementation copied from
 * arch/arm64/include/asm/spinlock.h in Linux
 *
 * Copyright (C) 2012 ARM Ltd.
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 */

#ifndef _GRINCH_ASM_SPINLOCK_H
#define _GRINCH_ASM_SPINLOCK_H

#include <grinch/types.h>

typedef struct { u32 lock; } spinlock_t __attribute__((aligned(4)));

#define DEFINE_SPINLOCK(X)	spinlock_t X = { 0 }

#define SPIN_LOCK_UNLOCKED	{ 0 }

static inline void spin_init(spinlock_t *lock)
{
	/* IMPLEMENT ME! */
	for (;;);
}

static inline void spin_lock(spinlock_t *lock)
{
	/* IMPLEMENT ME! */
	for (;;);
}

static inline void spin_unlock(spinlock_t *lock)
{
	/* IMPLEMENT ME! */
	for (;;);
}

#endif /* !_JAILHOUSE_ASM_SPINLOCK_H */
