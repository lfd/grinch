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

#ifndef _IRQCHIP_H
#define _IRQCHIP_H

#include <grinch/errno.h>

#define	IRQ_INVALID	((u32)-1)

struct device;

#define IRQ_NONE	0
#define IRQ_HANDLED	1

typedef int (*irq_handler_t)(void *userdata);

#define IRQ_SHARED	(1 << 0)

struct irq_action {
	irq_handler_t handler;
	void *userdata;
	unsigned int flags;
	struct irq_action *next;
};

struct irqchip_fn {
	void (*handle_irq)(void);
	int (*enable_irq)(unsigned int irq);
	int (*disable_irq)(unsigned int irq);
	int (*set_affinity)(unsigned int irq, unsigned long cpu);
	int (*init)(struct device *dev);
	/*
	 * A line number out of the cells a tree spends on this chip. The
	 * property may carry several specifiers; decode takes the first.
	 */
	int (*decode)(const u32 *cells, unsigned int n);
	/* Send an inter-processor interrupt to a remote CPU. */
	void (*send_ipi)(unsigned long cpuid);
	/* Per-CPU controller setup, run on every CPU that comes online. */
	int (*cpu_init)(void);
};

extern const struct irqchip_fn *irqchip_fn;

int irqchip_init(void);
int irqchip_decode(const u32 *cells, unsigned int n);
int irq_register_action(u32 irq, struct irq_action *act);
void irq_unregister_action(u32 irq, struct irq_action *act);
void irqchip_handle_irq(unsigned int irq);
int irqchip_enable_irq(unsigned int irq);
int irqchip_set_affinity(unsigned int irq, unsigned long cpu);
void irqchip_send_ipi(unsigned long cpuid);

/*
 * Per-CPU interrupt setup, run on every CPU as it comes online: the arch's
 * local interrupt bring-up (arch_irqchip_cpu_init) plus, if present, the
 * external controller's per-CPU init.
 */
int irqchip_cpu_init(void);

#include <asm/irqchip.h>

#endif /* _IRQCHIP_H */
