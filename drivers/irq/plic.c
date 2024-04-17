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

#define dbg_fmt(x) "plic: " x

#include <asm/cpu.h>

#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/mmio.h>

#define CTX_MAX		32

static void *plic;

static inline u16 this_ctx(void)
{
	return this_per_cpu()->plic.ctx;
}

static inline void plic_write_reg(u32 reg, u32 value)
{
	mmio_write32(plic + reg, value);
}

static inline u32 plic_read_reg(u32 reg)
{
	return mmio_read32(plic + reg);
}

static inline void plic_irq_set_prio(u32 irq, u32 prio)
{
	plic_write_reg(4 * irq, prio);
}

static inline void plic_irq_set_prio_thres(u16 ctx, u32 irq, u32 thres)
{
	plic_write_reg(0x200000 + ctx * 0x1000, thres);
}

static inline void plic_irq_set_enable(u16 ctx, u32 irq, bool enable)
{
	u32 reg = 0x2000 + ctx * 0x80 + (irq / 32) * 4;
	u32 value;

	value = plic_read_reg(reg);

	if (enable)
		value |= (1 << (irq % 32));
	else
		value &= ~(1 << (irq % 32));

	plic_write_reg(reg, value);
}

static int plic_disable_irq(unsigned long hart, u32 irq)
{
	plic_irq_set_enable(per_cpu(hart)->plic.ctx, irq, false);

	return 0;
}

static int plic_enable_irq(unsigned long hart, u32 irq, u32 prio, u32 thres)
{
	unsigned int ctx = per_cpu(hart)->plic.ctx;

	plic_irq_set_prio(irq, prio);
	plic_irq_set_prio_thres(ctx, irq, thres);

	plic_irq_set_enable(ctx, irq, true);
	return 0;
}

static void plic_handle_irq(void)
{
	u32 source;

	/* read source */
	source = plic_read_reg(0x200000 + 0x4 + 0x1000 * this_ctx());

	if (source == 0) {
		pr("Sprurious IRQ!\n");
		return;
	}

	irqchip_handle_irq(source);

	/* indicate completion */
	plic_write_reg(0x200000 + 0x4 + 0x1000 * this_ctx(), source);
}

static int __init plic_init(struct device *dev, void *vaddr)
{
	struct per_cpu *pcpu = this_per_cpu();
	unsigned int irq;

	plic = vaddr;

	pcpu->plic.ctx = this_cpu_id() * 2 + 1;

	for (irq = 0; irq < IRQ_MAX; irq++)
		plic_disable_irq(pcpu->cpuid, irq);

	return 0;
}

const struct irqchip_fn irqchip_fn_plic = {
	.handle_irq = plic_handle_irq,
	.enable_irq = plic_enable_irq,
	.disable_irq = plic_disable_irq,
	.init = plic_init,
};
