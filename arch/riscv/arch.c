/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"arch: " x

#include <asm/firmware.h>

#include <grinch/arch.h>
#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/hypercall.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/reboot.h>
#include <grinch/timer.h>

#include <grinch/arch/vmm.h>

static int guest_shutdown(int err)
{
	return hypercall_vmquit(err);
}

static int guest_reboot(void)
{
	/* No reboot hypercall yet -- fall back to halt */
	return hypercall_vmquit(0);
}

int __init arch_init(void)
{
	int err;

	err = firmware_init();
	if (err)
		goto out;

	if (grinch_is_guest) {
		arch_shutdown = guest_shutdown;
		arch_reboot = guest_reboot;
	}

	/* Boot secondary CPUs */
	pri("Booting secondary CPUs\n");
	/* Enable our IPI before starting secondaries so we can receive theirs. */
	arch_irqchip_cpu_init();
	err = smp_init();
	if (err)
		goto out;

	err = timer_init();
	if (err)
		goto out;

	err = vmm_init();
	if (err)
		goto out;

out:
	return err;
}
