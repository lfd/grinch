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

#ifndef _IRQCHIP_H
#define _IRQCHIP_H

#include <grinch/errno.h>

#define	IRQ_INVALID	((u32)-1)

struct device;

typedef int (*irq_handler_t)(void *userdata);

struct irqchip_fn {
	void (*handle_irq)(void);
	int (*enable_irq)(unsigned int irq);
	int (*disable_irq)(unsigned int irq);
	int (*set_affinity)(unsigned int irq, unsigned long cpu);
	int (*init)(struct device *dev);
	/* Per-CPU controller setup, run on every CPU that comes online. */
	int (*cpu_init)(void);
};

extern const struct irqchip_fn *irqchip_fn;

int irqchip_init(void);
int irq_register_handler(u32 irq, irq_handler_t handler, void *userdata);
void irqchip_handle_irq(unsigned int irq);
int irqchip_enable_irq(unsigned int irq);
int irqchip_set_affinity(unsigned int irq, unsigned long cpu);

/*
 * Per-CPU interrupt setup, run on every CPU as it comes online: the arch's
 * local interrupt bring-up (arch_irqchip_cpu_init) plus, if present, the
 * external controller's per-CPU init.
 */
int irqchip_cpu_init(void);

#include <asm/irqchip.h>

#endif /* _IRQCHIP_H */
