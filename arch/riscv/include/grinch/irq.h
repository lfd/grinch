/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
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

void irq_init(unsigned long timerdelta);
int handle_irq(u64 irq);
int handle_ipi(void);
int handle_timer(void);

static inline u64 get_time(void)
{
	return csr_read(time);
}

static inline void ext_enable(void)
{
	csr_set(sie, IE_EIE);
}

static inline void ext_disable(void)
{
	csr_clear(sie, IE_EIE);
}

static inline void ipi_enable(void)
{
	csr_set(sie, IE_SIE);
}

static inline void ipi_disable(void)
{
	csr_clear(sie, IE_SIE);
}

static inline void timer_enable(void)
{
	csr_set(sie, IE_TIE);
}

static inline void timer_disable(void)
{
	csr_clear(sie, IE_TIE);
}

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
