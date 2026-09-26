/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _GRINCH_SPINLOCK_H
#define _GRINCH_SPINLOCK_H

#include <asm/irq.h>
#include <asm/spinlock.h>

static inline unsigned long spin_lock_irqsave(spinlock_t *lock)
{
	unsigned long flags;

	flags = irq_save();
	spin_lock(lock);

	return flags;
}

static inline void
spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
	spin_unlock(lock);
	irq_restore(flags);
}

#endif /* _GRINCH_SPINLOCK_H */
