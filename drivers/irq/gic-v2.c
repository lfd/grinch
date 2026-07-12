/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "gic-v2: " x

#include <grinch/device.h>
#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/ioremap.h>
#include <grinch/irqchip.h>
#include <grinch/mmio.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/iores.h>

#define GICC_CTLR               0x0000
#define GICC_PMR                0x0004
#define GICC_IAR                0x000c
#define GICC_EOIR               0x0010

#define GICD_CTLR               0x0000
#define  GICD_CTLR_ENABLE       (1 << 0)
#define GICD_ISENABLER		0x0100
#define GICD_IPRIORITYR		0x0400
#define GICD_ITARGETSR		0x0800
#define GICD_SGIR		0x0f00

#define GICC_CTLR_GRPEN1        (1 << 0)

#define GICC_PMR_DEFAULT        0xf0

/*
 * Shared peripheral interrupts start at INTID 32; below that are the
 * per-CPU banked SGIs and PPIs.
 */
#define GIC_SPI_BASE		32
#define GIC_PRIO_DEFAULT	0xa0

/* SGI reserved for inter-processor interrupts. */
#define IPI_SGI			0

struct mmio_resource gicd, gicc;

static int gic_v2_enable_irq(u32 irq)
{
	/* ISENABLER is banked per CPU for SGIs/PPIs and shared for SPIs. */
	mmio_write32(gicd.base + GICD_ISENABLER + (irq / 32) * 4,
		     1 << (irq % 32));
	return 0;
}

static int gic_v2_disable_irq(u32 irq)
{
	return -ENOSYS;
}

static int gic_v2_set_affinity(u32 irq, unsigned long cpu)
{
	/* Only SPIs are routable; SGIs/PPIs are per-CPU banked. */
	if (irq < GIC_SPI_BASE)
		return -EINVAL;

	/* ITARGETSR holds one CPU-interface bitmask byte per IRQ. */
	mmio_write8(gicd.base + GICD_ITARGETSR + irq, 1 << cpu);
	return 0;
}

static void gic_v2_handle_irq(void)
{
	u32 irq;

	while (1) {
		irq = mmio_read32(gicc.base + GICC_IAR) & 0x3ff;
		if (irq == 0x3ff)
			break;

		irqchip_handle_irq(irq);
		mmio_write32(gicc.base + GICC_EOIR, irq);
	}
}

static int gic_v2_ipi_handler(void *unused)
{
	check_events();
	return 0;
}

static void gic_v2_send_ipi(unsigned long cpuid)
{
	/* TargetListFilter 0b00: deliver to the CPU interface in the list. */
	mmio_write32(gicd.base + GICD_SGIR,
		     (1u << (16 + cpuid)) | IPI_SGI);
}

/* Per-CPU interface bring-up; run on the boot CPU and every secondary. */
static int gic_v2_cpu_init(void)
{
	/* SGI enable bits (GICD_ISENABLER0) are banked per CPU. */
	mmio_write32(gicd.base + GICD_ISENABLER, 1 << IPI_SGI);
	mmio_write32(gicc.base + GICC_PMR, GICC_PMR_DEFAULT);
	mmio_write32(gicc.base + GICC_CTLR, GICC_CTLR_GRPEN1);

	return 0;
}

static int irq_gic_v2_init(void)
{
	unsigned int irq;
	int err;

	pr("Initialising interrupt controller GIC-V2\n");

	err = ioremap_res(&gicd);
	if (err)
		return err;

	err = ioremap_res(&gicc);
	if (err) {
		iounmap_res(&gicd);
		return err;
	}

	/* Enable the distributor (global) once. */
	mmio_write32(gicd.base + GICD_CTLR, GICD_CTLR_ENABLE);

	/*
	 * Give every SPI a default priority and route it to the boot CPU, so
	 * enable_irq() only has to unmask it.
	 */
	for (irq = GIC_SPI_BASE; irq < IRQ_MAX; irq++) {
		mmio_write8(gicd.base + GICD_IPRIORITYR + irq, GIC_PRIO_DEFAULT);
		mmio_write8(gicd.base + GICD_ITARGETSR + irq, 1 << 0);
	}

	err = irq_register_handler(IPI_SGI, gic_v2_ipi_handler, NULL);
	if (err)
		return err;

	/* Bring up this (boot) CPU's interface. */
	gic_v2_cpu_init();

	return 0;
}

const struct irqchip_fn irqchip_fn_gic_v2 = {
	.handle_irq = gic_v2_handle_irq,
	.enable_irq = gic_v2_enable_irq,
	.disable_irq = gic_v2_disable_irq,
	.set_affinity = gic_v2_set_affinity,
	.send_ipi = gic_v2_send_ipi,
	.cpu_init = gic_v2_cpu_init,
};

static const __initconst struct of_device_id gic_v2_compats[] = {
	{ .compatible = "arm,cortex-a15-gic", .data = &irqchip_fn_gic_v2, },
	{ /* sentinel */ }
};

static int __init gic_v2_probe(struct device *dev)
{
	int err;

	err = fdt_read_reg(_fdt, dev->of.node, 0, &gicd.phys);
	if (err)
		return err;

	err = fdt_read_reg(_fdt, dev->of.node, 1, &gicc.phys);
	if (err)
		return err;

	pri("Found GIC-V2: "
	    "GICD Base/SZ: 0x%lx/0x%lx, GICC Base/SZ: 0x%lx/0x%lx\n",
	    gicd.phys.paddr, gicd.phys.size, gicc.phys.paddr, gicc.phys.size);

	err = irq_gic_v2_init();
	if (err)
		return err;

	irqchip_fn = &irqchip_fn_gic_v2;

	return 0;
}

DECLARE_IRQCHIP(GICV2, "GICv2", gic_v2_probe, gic_v2_compats);
