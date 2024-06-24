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
	int err;

	irqchip_fn = (const struct irqchip_fn *)(dev->of.match->data);

	err = dev_map_iomem(dev);
	if (err)
		return err;

	err = irqchip_fn->init(dev);
	if (err)
		return err;

	ext_enable();

	return 0;
}

DECLARE_DRIVER(PLIC, "PLIC", PRIO_0, NULL, xplic_probe, plic_compats);
DECLARE_DRIVER(APLIC, "APLIC", PRIO_0, NULL, xplic_probe, aplic_compats);
