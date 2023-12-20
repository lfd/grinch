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

#define dbg_fmt(x)	"arch: " x

#include <asm/irq.h>

#include <grinch/arch.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/hypercall.h>
#include <grinch/paging.h>
#include <grinch/sbi.h>
#include <grinch/serial.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/kmm.h>
#include <grinch/pmm.h>
#include <grinch/alloc.h>

int arch_init(paddr_t __fdt)
{
	int err;

	err = fdt_init(__fdt);
	if (err)
		goto out;

	err = platform_init();
	if (err)
		goto out;

	err = pmm_init_fdt();
	if (err)
		goto out;

	err = kheap_init();
	if (err)
		goto out;

	err = sbi_init();
	if (err)
		goto out;

	ps("Disabling IRQs\n");
	irq_disable();
	timer_disable();

	/* Initialise external interrupts */
	ps("Initialising irqchip...\n");
	err = irqchip_init();
	if (err == -ENOENT)
		pr("No irqchip found!\n");
	else if (err)
		goto out;
	else
		ext_enable();

	ps("Initialising Serial...\n");
	err = serial_init_fdt();
	if (err)
	{
		if (err == -ENOENT)
			goto con;
		goto out;
	}
	ps("Switched over from SBI to UART\n");
con:

	/* Boot secondary CPUs */
	ps("Booting secondary CPUs\n");
	err = smp_init();
	if (err)
		goto out;

#if 0
	ps("Enabling IRQs\n");
	irq_enable();
#endif

out:
	return err;
}

void __noreturn arch_shutdown(void)
{
	if (grinch_is_guest) {
		hypercall_vmquit(0);
		panic("Unreachable!\n");
	}

	panic("Shutdown.\n");
}
