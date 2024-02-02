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
#include <asm/isa.h>

#include <grinch/arch.h>
#include <grinch/gfp.h>
#include <grinch/hypercall.h>
#include <grinch/serial.h>
#include <grinch/irqchip.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/alloc.h>
#include <grinch/vfs.h>

#include <grinch/arch/sbi.h>
#include <grinch/arch/vmm.h>

int __init arch_init(void)
{
	int err;

	err = platform_init();
	if (err)
		goto out;

	err = phys_mem_init_fdt();
	if (err)
		goto out;

	err = initrd_init_early();
	if (err == -ENOENT)
		pri("No ramdisk found\n");
	else if (err)
		goto out;

	err = kheap_init();
	if (err)
		goto out;

	err = sbi_init();
	if (err)
		goto out;

	psi("Disabling IRQs\n");
	irq_disable();
	timer_disable();

	/* Initialise external interrupts */
	psi("Initialising irqchip...\n");
	err = irqchip_init();
	if (err == -ENOENT)
		pri("No irqchip found!\n");
	else if (err)
		goto out;
	else
		ext_enable();

	psi("Initialising Serial...\n");
	err = serial_init_fdt();
	if (err)
	{
		if (err == -ENOENT)
			goto con;
		goto out;
	}
	psi("Switched over from SBI to UART\n");
con:

	/* Boot secondary CPUs */
	psi("Booting secondary CPUs\n");
	err = smp_init();
	if (err)
		goto out;

#if 0
	ps("Enabling IRQs\n");
	irq_enable();
#endif

	err = vmm_init();
	if (err == -ENOSYS) {
		psi("H-Extensions not available\n");
		err = 0;
	}

	if (1 && has_hypervisor()) {
		/* create two test instances */
		err = vm_create_grinch();
		if (err)
			goto out;
#if 0
		err = vm_create_grinch();
		if (err)
			goto out;
#endif
	}

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
