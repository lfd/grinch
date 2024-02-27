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

#define dbg_fmt(x)	"arch: " x

#include <asm/irq.h>

#include <grinch/arch.h>
#include <grinch/alloc.h>
#include <grinch/gfp.h>
#include <grinch/hypercall.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/panic.h>
#include <grinch/platform.h>
#include <grinch/printk.h>
#include <grinch/serial.h>
#include <grinch/timer.h>

#include <grinch/arch/sbi.h>

int __init arch_init(void)
{
	int err;

	irq_disable();

	err = kheap_init();
	if (err)
		goto out;

	err = platform_init();
	if (err)
		goto out;

	err = sbi_init();
	if (err)
		goto out;

	/* Initialise external interrupts */
	pri("Initialising irqchip...\n");
	err = irqchip_init();
	if (err == -ENOENT)
		pri("No irqchip found!\n");
	else if (err)
		goto out;
	else
		ext_enable();

	pri("Initialising Serial...\n");
	err = serial_init_fdt();
	if (err)
	{
		if (err == -ENOENT)
			goto con;
		goto out;
	}
	pri("Switched over from SBI to UART\n");
con:

	/* Boot secondary CPUs */
	pri("Booting secondary CPUs\n");
	ipi_enable();
	err = smp_init();
	if (err)
		goto out;

	err = timer_init();
	if (err)
		goto out;

	err = vmm_init();
	if (err == -ENOSYS) {
		pri("H-Extensions not available\n");
		err = 0;
	}

out:
	return err;
}

void __noreturn arch_shutdown(int err)
{
	if (grinch_is_guest) {
		hypercall_vmquit(err);
		BUG();
	}

	panic("Shutdown. Reason: %pe\n", ERR_PTR(err));
}
