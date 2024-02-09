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

#include <grinch/errno.h>
#include <grinch/irqchip.h>
#include <grinch/printk.h>

static irq_handler_t irq_handlers[IRQ_MAX];
static void *irq_handlers_userdata[IRQ_MAX];

int irq_register_handler(u32 irq, irq_handler_t handler, void *userdata)
{
	if (!handler)
		return -EINVAL;

	if (irq >= IRQ_MAX)
		return -EINVAL;

	irq_handlers[irq] = handler;
	irq_handlers_userdata[irq] = userdata;

	return 0;
}

void irqchip_handle_irq(unsigned int irq)
{
	irq_handler_t handler = NULL;
	int err;

	if (irq < IRQ_MAX)
		handler = irq_handlers[irq];

	if (handler) {
		err = handler(irq_handlers_userdata[irq]);
		if (err)
			pr("Handler error for IRQ %u: %d\n", irq, err);
	} else
		pr("No Handler for IRQ %u\n", irq);
}
