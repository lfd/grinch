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

#include <asm/csr.h>

#include <grinch/errno.h>
#include <grinch/printk.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/irqchip.h>

extern const struct irqchip_fn irqchip_fn_plic;
extern const struct irqchip_fn irqchip_fn_aplic;

struct irqchip irqchip;

static irq_handler_t irq_handlers[IRQ_MAX];
static void *irq_handlers_userdata[IRQ_MAX];

static const struct of_device_id plic_compats[] = {
	{ .compatible = "riscv,plic0", .data = &irqchip_fn_plic, },
	{ .compatible = "sifive,plic-1.0.0", .data = &irqchip_fn_plic, },
	{ /* sentinel */ }
};

static const struct of_device_id aplic_compats[] = {
	{ .compatible = "riscv,aplic", .data = &irqchip_fn_aplic, },
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

static int fdt_irqchip_get_extended(const void *fdt, int off)
{
	const fdt32_t *reg;
	int res;

	reg = fdt_getprop(fdt, off, "interrupts-extended", &res);
	if (res < 0)
		return res;

	return fdt32_to_cpu(reg[1]);
}

int irqchip_init(void)
{
	int err, off;
	const struct of_device_id *match;

	/* Probe for PLIC */
	off = fdt_find_device(_fdt, "/soc", plic_compats, &match);
	if (off >= 0)
		goto init;

	/* Probe for APLIC */
	off = fdt_find_device(_fdt, "/soc", aplic_compats, &match);
	if (off < 0)
		return off;

	err = fdt_irqchip_get_extended(_fdt, off);
	if (err != IRQ_S_EXT)
		return -EINVAL;

	/* Initialise IRQ controller */
init:
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
