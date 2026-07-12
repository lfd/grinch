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

#define IRQ_MAX		64

extern const struct irqchip_fn irqchip_fn_gic_v2;

/*
 * No separate local interrupt controller: the GIC's cpu_init brings up the
 * CPU interface and enables the IPI SGI, so there is nothing to do here.
 */
static inline void arch_irqchip_cpu_init(void) { }
