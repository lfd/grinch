/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "main: " x

#include <grinch/fdt.h>
#include <grinch/mm.h>
#include <grinch/printk.h>
#include <grinch/paging.h>
#include <grinch/sbi.h>
#include <grinch/symbols.h>
#include <grinch/irq.h>

void cmain(paddr_t __fdt);

/* not used for guests */
void secondary_cmain(void);
void secondary_cmain(void)
{
	for (;;);
}

void cmain(paddr_t __fdt)
{
	int err;

	ps("Hello, world. I'm a virtual machine!\n");
	pr("My stack is here: %p\n", &err);

	err = mm_init();
	if (err)
		goto out;

	ps("Activating MMU...\n");
	err = paging_init();
	if (err)
		goto out;
	ps("Success!\n");

	err = fdt_init(__fdt);
	if (err)
		goto out;

	err = mm_init_late();
	if (err)
		goto out;

	ps("Enabling Timer + IRQ\n");
	timer_enable();
	irq_enable();

	/* Also enable Soft IRQs for testing purposes */
	ipi_enable();
	ext_enable();

	sbi_set_timer(get_time() + 0x400000);

	ps("Enabling FPU...\n");
	csr_set(sstatus, SR_SD | SR_FS_DIRTY | SR_XS_DIRTY);

	ps("Testing FPU...\n");
	asm volatile("mv	a0, %0\n"
			".word   0x00053007\n"
		     : : "r"(_load_addr): "a0", "ft0", "memory");

	ps("End reached.\n");
out:	
	if (err)
		pr("Error occured: %d\n", err);
}
