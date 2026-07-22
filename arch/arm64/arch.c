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

#define dbg_fmt(x)	"arch: " x

#include <asm/psci.h>

#include <grinch/arch.h>
#include <grinch/gfp.h>
#include <grinch/irqchip.h>
#include <grinch/panic.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/serial.h>
#include <grinch/smp.h>

void __noreturn arch_reboot(void)
{
	panic("Reboot not implemented\n");
}

void __noreturn arch_shutdown(int err)
{
	panic("Shutdown. Reason: %pe\n", ERR_PTR(err));
}

int __init arch_init(void)
{
	int err;

	psci_init();

	/*
	 * Bring up the secondaries before timer_init(): the latter fans out
	 * timer_cpu_init() via on_each_cpu(), which only reaches CPUs that
	 * are already online and servicing IPIs.
	 */
	err = smp_init();
	if (err)
		return err;

	err = timer_init();
	if (err)
		return err;

	return 0;
}
