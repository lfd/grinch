/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
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
static inline bool ext_pending(void)
{
	return !!(csr_read(sip) & IE_EIE);
}

static inline void ext_enable(void)
{
	csr_set(sie, IE_EIE);
}

static inline void ext_disable(void)
{
	csr_clear(sie, IE_EIE);
}

/* Helpers for IPIs */
static inline void ipi_enable(void)
{
	csr_set(sie, IE_SIE);
}

static inline void ipi_disable(void)
{
	csr_clear(sie, IE_SIE);
}

static inline void ipi_clear(void)
{
	csr_clear(sip, IE_SIE);
}

static inline bool ipi_pending(void)
{
	return !!(csr_read(sip) & IE_SIE);
}

/* Helpers for timers */
static inline bool timer_pending(void)
{
	return !!(csr_read(sip) & IE_TIE);
}

static inline void timer_enable(void)
{
	csr_set(sie, IE_TIE);
}

static inline void timer_disable(void)
{
	csr_clear(sie, IE_TIE);
}

/* Local IRQ control */
static inline void irq_disable(void)
{
	csr_clear(sstatus, SR_SIE);
}

static inline void irq_enable(void)
{
	csr_set(sstatus, SR_SIE);
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
