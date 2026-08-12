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

#ifndef _IRQ_H
#define _IRQ_H

#include <asm/csr.h>
#include <grinch/types.h>

/* Helpers for external IRQs */
static inline void ext_enable(void)
{
	csr_set(CSR_IE, IE_EXT);
}

static inline void ext_disable(void)
{
	csr_clear(CSR_IE, IE_EXT);
}

/* Helpers for IPIs */
static inline void ipi_enable(void)
{
	csr_set(CSR_IE, IE_SOFT);
}

static inline void ipi_disable(void)
{
	csr_clear(CSR_IE, IE_SOFT);
}

static inline void ipi_clear(void)
{
	csr_clear(CSR_IP, IE_SOFT);
}

/* Helpers for timers */
static inline void timer_enable(void)
{
	csr_set(CSR_IE, IE_TIMER);
}

static inline void timer_disable(void)
{
	csr_clear(CSR_IE, IE_TIMER);
}

/* Local IRQ control */
static inline void irq_disable(void)
{
	csr_clear(CSR_STATUS, SR_IE);
}

static inline void irq_enable(void)
{
	csr_set(CSR_STATUS, SR_IE);
}

static inline bool is_irq(u64 cause)
{
	return !!(cause & CAUSE_IRQ_FLAG);
}

static inline unsigned long to_irq(unsigned long cause)
{
	return cause & ~CAUSE_IRQ_FLAG;
}

#endif /* _IRQ_H */
