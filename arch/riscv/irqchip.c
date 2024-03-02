/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
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

int __init irqchip_init(void)
{
	int err, off;
	const struct of_device_id *match;

	/* Probe for PLIC */
	off = fdt_find_device(_fdt, ISTR("/soc"), plic_compats, &match);
	if (off >= 0)
		goto init;

	/* Probe for APLIC */
	off = fdt_find_device(_fdt, ISTR("/soc"), aplic_compats, &match);
	if (off < 0)
		return -ENOENT;

	err = fdt_irqchip_get_extended(_fdt, off);
	if (err != IRQ_S_EXT)
		return -EINVAL;

	/* Initialise IRQ controller */
init:
	irqchip_fn = (const struct irqchip_fn *)(match->data);

	paddr_t pbase;
	void *vbase;
	u64 size;

	err = fdt_read_reg(_fdt, off, 0, &pbase, &size);
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

	return err;
}
