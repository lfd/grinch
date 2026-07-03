/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _IRQ_H
#define _IRQ_H

#include <asm/sysregs.h>

static inline void irq_enable(void)
{
	asm volatile("msr daifclr, #(" __stringify(DAIF_IRQ | DAIF_FIQ) ")");
}

static inline void irq_disable(void)
{
	asm volatile("msr daifset, #(" __stringify(DAIF_IRQ | DAIF_FIQ) ")");
}

/*
 * On ARM64, the virtual timer PPI (IRQ 27) is enabled in the GIC via
 * GICD_ISENABLER, which is CPU-banked for the PPI range. For SMP,
 * each CPU must enable it from its own context (e.g. in an
 * arch_timer_cpu_init hook). For now we handle it once in
 * arch_timer_init() which runs on CPU 0.
 */
static inline void timer_enable(void)
{
}

#endif /* _IRQ_H */
