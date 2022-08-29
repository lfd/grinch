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

#define dbg_fmt(x) "aplic: " x

#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/mmio.h>

static void aplic_enable_irq(unsigned long hart, u32 irq, u32 prio, u32 thres)
{
}

static void aplic_disable_irq(unsigned long hart, u32 irq)
{
}

static int aplic_handle_irq(void)
{
	return -EINVAL;
}

static void aplic_hart_init(void)
{
}

const struct irqchip_fn irqchip_fn_aplic = {
	.hart_init = aplic_hart_init,
	.handle_irq = aplic_handle_irq,

	.enable_irq = aplic_enable_irq,
	.disable_irq = aplic_disable_irq,
};
