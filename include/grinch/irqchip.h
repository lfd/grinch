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

#ifndef _IRQCHIP_H
#define _IRQCHIP_H

#define IRQ_MAX		32

typedef int (*irq_handler_t)(void *userdata);

struct irqchip_fn {
	void (*hart_init)(void);
	int (*handle_irq)(void);

	void (*enable_irq)(unsigned long hart, u32 irq, u32 prio, u32 thres);
	void (*disable_irq)(unsigned long hart, u32 irq);
};

struct irqchip {
	paddr_t pbase;
	void *vbase;
	u64 size;

	const struct irqchip_fn *fn;
};

extern struct irqchip irqchip;

int irq_register_handler(u32 irq, irq_handler_t handler, void *userdata);
int irqchip_handle_irq(unsigned int irq);
int irqchip_init(void);

static inline void
irqchip_enable_irq(unsigned long hart, u32 irq, u32 prio, u32 thres)
{
	irqchip.fn->enable_irq(hart, irq, prio, thres);
}

#endif /* _IRQCHIP_H */
