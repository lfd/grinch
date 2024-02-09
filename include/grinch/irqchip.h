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

typedef int (*irq_handler_t)(void *userdata);

struct irqchip_fn {
	void (*handle_irq)(void);
	int (*enable_irq)(unsigned long cpuid, u32 irq, u32 prio, u32 thres);
	int (*disable_irq)(unsigned long cpuid, u32 irq);
};

extern const struct irqchip_fn *irqchip_fn;

int irq_register_handler(u32 irq, irq_handler_t handler, void *userdata);
void irqchip_handle_irq(unsigned int irq);
int irqchip_init(void);

static inline int
irqchip_enable_irq(unsigned long cpuid, u32 irq, u32 prio, u32 thres)
{
	if (irqchip_fn)
		return irqchip_fn->enable_irq(cpuid, irq, prio, thres);

	return -ENOENT;
}

#include <asm/irqchip.h>

#endif /* _IRQCHIP_H */
