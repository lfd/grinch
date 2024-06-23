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

#define dbg_fmt(x) "xplic: " x

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

static int __init xplic_probe(struct device *dev)
{
	void *vbase;
	int err;

	irqchip_fn = (const struct irqchip_fn *)(dev->of.match->data);

	err = fdt_read_reg(_fdt, dev->of.node, 0, &dev->mmio);
	if (err)
		return err;

	pri("base: 0x%llx, size: 0x%lx\n", (u64)dev->mmio.paddr, dev->mmio.size);

	vbase = ioremap(dev->mmio.paddr, dev->mmio.size);
	if (IS_ERR(vbase))
		return PTR_ERR(vbase);

	err = irqchip_fn->init(dev, vbase);
	if (err)
		return err;

	ext_enable();

	return 0;
}

DECLARE_DRIVER(PLIC, "PLIC", PRIO_0, NULL, xplic_probe, plic_compats);
DECLARE_DRIVER(APLIC, "APLIC", PRIO_0, NULL, xplic_probe, aplic_compats);
