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

#include <grinch/errno.h>
#include <grinch/printk.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/irqchip.h>

extern const struct irqchip_fn irqchip_fn_plic;

struct irqchip irqchip;

static irq_handler_t irq_handlers[IRQ_MAX];
static void *irq_handlers_userdata[IRQ_MAX];

static const struct of_device_id plic_compats[] = {
	{ .compatible = "riscv,plic0", .data = &irqchip_fn_plic, },
	{ .compatible = "sifive,plic-1.0.0", .data = &irqchip_fn_plic, },
	{ /* sentinel */ }
};

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

int irqchip_handle_irq(unsigned int irq)
{
	irq_handler_t handler = NULL;
	int err;

	if (irq < IRQ_MAX)
		handler = irq_handlers[irq];

	if (handler)
		err = handler(irq_handlers_userdata[irq]);
	else {
		pr("No Handler for PLIC IRQ %u\n", irq);
		err = -ENOENT;
	}

	return err;
}

int irqchip_init(void)
{
	int err, off;
	const struct of_device_id *match;

	off = fdt_find_device(_fdt, "/soc", plic_compats, &match);
	if (off < 0)
		return off;

	irqchip.fn = (const struct irqchip_fn *)(match->data);

	err = fdt_read_reg(_fdt, off, 0, &irqchip.pbase, &irqchip.size);
	if (err)
		return err;

	pr("base: 0x%llx, size: 0x%llx\n", (u64)irqchip.pbase, irqchip.size);

	irqchip.vbase = ioremap(irqchip.pbase, irqchip.size);
	if (IS_ERR(irqchip.vbase))
		return PTR_ERR(irqchip.vbase);

	irqchip.fn->hart_init();

	return 0;
}
