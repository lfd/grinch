/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "native: " x

#include <asm/clint.h>
#include <asm/cpu.h>
#include <asm/firmware.h>

#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/init.h>
#include <grinch/ioremap.h>
#include <grinch/panic.h>
#include <grinch/printk.h>

/*
 * Machine mode has no firmware below it: the kernel drives the timer, IPI
 * and reset hardware itself. Until those drivers exist, say which service
 * was missing rather than trapping somewhere unrelated.
 */
#define todo(service)	panic("no machine mode " service " yet\n")

void *clint;

static __initconst const struct of_device_id clint_compats[] = {
	{ .compatible = "riscv,clint0", },
	{ .compatible = "sifive,clint0", },
	{ /* sentinel */ }
};

int __init clint_init(void)
{
	struct mmio_area area;
	int off, err;

	off = fdt_find_device(_fdt, ISTR("/soc"), clint_compats, NULL);
	if (off < 0)
		return off;

	err = fdt_read_reg(_fdt, off, 0, &area);
	if (err)
		return err;

	clint = ioremap(area.paddr, area.size);
	if (IS_ERR(clint))
		return PTR_ERR(clint);

	pri("CLINT at 0x%llx, size 0x%lx\n", (u64)area.paddr, area.size);

	return 0;
}

int __init firmware_init(void)
{
	pr_info_i("Running in machine mode\n");
	pr_warn_i("No reset handler: shutdown and reboot unavailable\n");

	return clint_init();
}

void firmware_timer_set(u64 ticks)
{
	clint_timer_set(ticks);
}

void firmware_ipi_send(unsigned long hmask)
{
	unsigned long hart;

	for (hart = 0; hmask; hmask >>= 1, hart++)
		if (hmask & 1)
			clint_ipi(hart, true);
}

/* Set by the boot code, where the harts we did not boot on are waiting. */
extern u32 hart_release;

int firmware_hart_start(unsigned long hart_id, paddr_t entry,
			unsigned long opaque)
{
	/* They resume at the secondary entry on their own, so only name it. */
	hart_release = hart_id;
	mb();

	return 0;
}

void firmware_remote_fence(unsigned long hmask, const void *addr, size_t size)
{
	todo("remote fence");
}

void firmware_remote_fence_asid(unsigned long hmask, unsigned long asid,
				const void *addr, size_t size)
{
	todo("remote fence");
}
