/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _IRQCHIP_H
#define _IRQCHIP_H

typedef int (*irq_handler_t)(void *userdata);

struct irqchip_fn {
	int (*handle_irq)(void);
	void (*enable_irq)(unsigned long cpuid, u32 irq, u32 prio, u32 thres);
	void (*disable_irq)(unsigned long cpuid, u32 irq);
};

extern const struct irqchip_fn *irqchip_fn;

int irq_register_handler(u32 irq, irq_handler_t handler, void *userdata);
int irqchip_handle_irq(unsigned int irq);
int irqchip_init(void);

static inline void
irqchip_enable_irq(unsigned long cpuid, u32 irq, u32 prio, u32 thres)
{
	irqchip_fn->enable_irq(cpuid, irq, prio, thres);
}

#include <asm/irqchip.h>

#endif /* _IRQCHIP_H */
