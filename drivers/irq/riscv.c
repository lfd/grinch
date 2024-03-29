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

#include <asm/csr.h>
#include <asm/irq.h>

#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/printk.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/irqchip.h>

const struct irqchip_fn *irqchip_fn;

static __initconst const struct of_device_id plic_compats[] = {
	{ .compatible = "riscv,plic0", .data = &irqchip_fn_plic, },
	{ .compatible = "sifive,plic-1.0.0", .data = &irqchip_fn_plic, },
	{ /* sentinel */ }
};

static __initconst const struct of_device_id aplic_compats[] = {
	{ .compatible = "riscv,aplic", .data = &irqchip_fn_aplic, },
	{ /* sentinel */ }
};

static int __init fdt_irqchip_get_extended(const void *fdt, int off)
{
	const fdt32_t *reg;
	int res;

	reg = fdt_getprop(fdt, off, "interrupts-extended", &res);
	if (res < 0)
		return res;

	return fdt32_to_cpu(reg[1]);
}

static int __init xplic_probe(struct device *dev)
{
	paddr_t pbase;
	int err;
	void *vbase;
	u64 size;

	irqchip_fn = (const struct irqchip_fn *)(dev->of.match->data);

	err = fdt_read_reg(_fdt, dev->of.node, 0, &pbase, &size);
	if (err)
		return err;

	pri("base: 0x%llx, size: 0x%llx\n", (u64)pbase, size);

	vbase = ioremap(pbase, size);
	if (IS_ERR(vbase))
		return PTR_ERR(vbase);

	if (irqchip_fn == &irqchip_fn_plic)
		err = plic_init(vbase);
	else if (irqchip_fn == &irqchip_fn_aplic)
		err = aplic_init(vbase);
	else
		return -ENOSYS;

	if (!err)
		ext_enable();

	return err;
}

DECLARE_DRIVER(PLIC, PRIO_0, NULL, xplic_probe, plic_compats);
DECLARE_DRIVER(APLIC, PRIO_0, NULL, xplic_probe, aplic_compats);
