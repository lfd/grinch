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

#include <asm/irq.h>

#define IRQ_MAX		64

extern const struct irqchip_fn irqchip_fn_plic;
extern const struct irqchip_fn irqchip_fn_aplic;

/*
 * Local per-hart interrupt bring-up. The IPI is a software interrupt in the
 * sie CSR, not an external-controller (PLIC/APLIC) source, so enable it here
 * rather than in the irqchip's cpu_init.
 */
static inline void arch_irqchip_cpu_init(void)
{
	ipi_enable();
}
