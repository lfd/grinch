/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "aplic: " x

#include <asm/cpu.h>

#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/xplic.h>

static int aplic_enable_irq(u32 irq)
{
	return -ENOSYS;
}

static int aplic_disable_irq(u32 irq)
{
	return -ENOSYS;
}

static void aplic_handle_irq(void)
{
}

static int __init aplic_init(struct device *dev)
{
	return -ENOSYS;
}

/* <hwirq flags>; the trigger mode the flags name is not ours to pick. */
static int aplic_decode(const u32 *cells, unsigned int n)
{
	if (n != 2)
		return -EINVAL;

	return fdt32_to_cpu(cells[0]);
}

const struct irqchip_fn irqchip_fn_aplic = {
	.handle_irq = aplic_handle_irq,
	.decode = aplic_decode,
	.enable_irq = aplic_enable_irq,
	.disable_irq = aplic_disable_irq,
	.init = aplic_init,
};

static __initconst const struct of_device_id aplic_compats[] = {
	{ .compatible = "riscv,aplic", .data = &irqchip_fn_aplic, },
	{ /* sentinel */ }
};

DECLARE_IRQCHIP(APLIC, "APLIC", xplic_probe, aplic_compats);
