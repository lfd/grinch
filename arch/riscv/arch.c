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
#include <grinch/errno.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/timer.h>

#include <grinch/arch/vmm.h>

int __init arch_init(void)
{
	int err;

	err = firmware_init();
	if (err)
		goto out;

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
