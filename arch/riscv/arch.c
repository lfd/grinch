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
#include <grinch/hypercall.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/timer.h>

#include <grinch/arch/sbi.h>

int __init arch_init(void)
{
	int err;

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
