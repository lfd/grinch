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

#include <grinch/device.h>
#include <grinch/fdt.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/mmio.h>

#define CTX_MAX		32

/*
 * PLIC priority 0 means "never interrupt", so enabled sources need a
 * non-zero priority. A per-context threshold of 0 lets every non-zero
 * priority through. We don't do priority ordering, so a uniform default
 * is enough.
 */
#define PLIC_PRIO_DEFAULT	1
#define PLIC_THRES_DEFAULT	0

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

static inline void plic_set_threshold(u16 ctx, u32 thres)
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

static int plic_disable_irq(u32 irq)
{
	plic_irq_set_enable(this_ctx(), irq, false);

	return 0;
}

static int plic_enable_irq(u32 irq)
{
	plic_irq_set_prio(irq, PLIC_PRIO_DEFAULT);
	plic_irq_set_enable(this_ctx(), irq, true);
	return 0;
}

static void plic_handle_irq(void)
{
	u32 source;

	/* read source */
	source = plic_read_reg(0x200000 + 0x4 + 0x1000 * this_ctx());

	if (source == 0) {
		pr_crit("Spurious IRQ!\n");
		return;
	}

	irqchip_handle_irq(source);

	/* indicate completion */
	plic_write_reg(0x200000 + 0x4 + 0x1000 * this_ctx(), source);
}

static int __init plic_get_context(const void *fdt, int off, unsigned int cpu)
{
	u32 cpu_phandle, phandle;
	const u32* iext;
	int entry, size, ic, err;

	ic = fdt_subnode_offset(fdt, per_cpu(cpu)->of_node,
				ISTR("interrupt-controller"));
	if (ic < 0) {
		pr_warn_i("No interrupt controller reference for CPU %u\n",
			  cpu);
		return ic;
	}

	err = fdt_read_u32(fdt, ic, ISTR("phandle"), &cpu_phandle);
	if (err)
		return err;

	iext = (const u32 *)
		fdt_getprop(fdt, off, ISTR("interrupts-extended"), &size);
	if (!iext)
		return -ENOENT;
	size /= sizeof(u32);

	for (entry = 0; entry < size; entry += 2) {
		if (fdt32_to_cpu(iext[entry + 1]) != 9)
			continue;

		phandle = fdt32_to_cpu(iext[entry]);
		if (phandle == cpu_phandle)
			return entry / 2;
	}

	return -ENOENT;
}

static int __init plic_init(struct device *dev)
{
	unsigned int cpu, irq;
	struct per_cpu *pcpu;
	int ctx;

	plic = dev->mmio.base;

	for_each_available_cpu(cpu) {
		pcpu = per_cpu(cpu);

		ctx = plic_get_context(_fdt, dev->of.node, cpu);
		if (ctx < 0) {
			dev_pri_warn(dev,
				     "Unable to get context for CPU %u: %pe\n",
				     cpu, ERR_PTR(ctx));
			return ctx;
		}
		pcpu->plic.ctx = ctx;

		plic_set_threshold(ctx, PLIC_THRES_DEFAULT);

		for (irq = 0; irq < IRQ_MAX; irq++)
			plic_irq_set_enable(ctx, irq, false);
	}

	return 0;
}

const struct irqchip_fn irqchip_fn_plic = {
	.handle_irq = plic_handle_irq,
	.enable_irq = plic_enable_irq,
	.disable_irq = plic_disable_irq,
	.init = plic_init,
};
