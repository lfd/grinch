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

#define dbg_fmt(x) "plic: " x

#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/mmio.h>
#include <grinch/plic.h>

#define PLIC_SIZE 	0x4000000
#define IRQ_MAX		32
#define CTX_MAX		32

#define PLIC_BASE	(void*)(0xf8000000)

struct plic {
	paddr_t pbase;
	void *vbase;
	u64 size;
} plic;

static irq_handler_t irq_handlers[IRQ_MAX];
static void *irq_handlers_userdata[IRQ_MAX];

static inline u16 this_ctx(void)
{
	return this_per_cpu()->plic.ctx;
}

static inline void plic_write_reg(u32 reg, u32 value)
{
	mmio_write32(plic.vbase + reg, value);
}

static inline u32 plic_read_reg(u32 reg)
{
	return mmio_read32(plic.vbase + reg);
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

void plic_disable_irq(unsigned long hart, u32 irq)
{
	plic_irq_set_enable(per_cpu(hart)->plic.ctx, irq, false);
}

void plic_enable_irq(unsigned long hart, u32 irq, u32 prio, u32 thres)
{
	unsigned int ctx = per_cpu(hart)->plic.ctx;
	plic_irq_set_prio(irq, prio);
	plic_irq_set_prio_thres(ctx, irq, thres);

	plic_irq_set_enable(ctx, irq, true);
}

int plic_register_handler(u32 irq, irq_handler_t handler, void *userdata)
{
	if (!handler)
		return -EINVAL;

	if (irq >= IRQ_MAX)
		return -EINVAL;

	irq_handlers[irq] = handler;
	irq_handlers_userdata[irq] = userdata;

	return 0;
}

int plic_handle_irq(void)
{
	int err = -ENOSYS;
	u32 source;
	irq_handler_t handler = NULL;

	/* read source */
	source = plic_read_reg(0x200000 + 0x4 + 0x1000 * this_ctx());

	if (source == 0)
	{
		pr("Sprurious IRQ!\n");
		return -EINVAL;
	}

	if (source < IRQ_MAX)
		handler = irq_handlers[source];

	if (handler)
		err = handler(irq_handlers_userdata[source]);
	else
		pr("No Handler for PLIC IRQ %u\n", source);

	/* indicate completion */
	plic_write_reg(0x200000 + 0x4 + 0x1000 * this_ctx(), source);
	return err;
}

static void plic_hart_init(void)
{
	struct per_cpu *pcpu = this_per_cpu();
	unsigned int irq;

	pcpu->plic.ctx = this_cpu_id() * 2 + 1;

	for (irq = 0; irq < IRQ_MAX; irq++)
		plic_disable_irq(pcpu->hartid, irq);
}

static const struct of_device_id plic_compats[] = {
	{ .compatible = "riscv,plic0" },
	{ .compatible = "sifive,plic-1.0.0"},
	{ /* sentinel */ }
};

int plic_init(void)
{
	int err, off;

	off = fdt_find_device(_fdt, "/soc", plic_compats, NULL);
	if (off < 0)
		return off;

	err = fdt_read_reg(_fdt, off, 0, &plic.pbase, &plic.size);
	if (err)
		return err;

	pr("base: 0x%llx, size: 0x%llx\n", (u64)plic.pbase, plic.size);

	plic.vbase = ioremap(plic.pbase, plic.size);
	if (IS_ERR(plic.vbase))
		return PTR_ERR(plic.vbase);

	plic_hart_init();

	return 0;
}


