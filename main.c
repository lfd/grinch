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

#define dbg_fmt(x)	"main: " x

#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/percpu.h>
#include <grinch/mm.h>
#include <grinch/plic.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/irq.h>
#include <grinch/serial.h>
#include <grinch/sbi.h>

void cmain(paddr_t fdt);

static const char logo[] =
"\n\n"
"            _            _\n"
"           (_)          | |\n"
"  __ _ _ __ _ _ __   ___| |_\n"
" / _` | '__| | '_ \\ / __| '_ \\\n"
"| (_| | |  | | | | | (__| | | |\n"
" \\__, |_|  |_|_| |_|\\___|_| |_|\n"
"  __/ |\n"
" |___/\n"
"      -> Welcome to Grinch " __stringify(GRINCH_VER) " <- \n\n\n";

void cmain(paddr_t __fdt)
{
	int err;

	_puts(logo);

	pr("Hartid: %lu\n", this_cpu_id());

	err = mm_init();
	if (err)
		goto out;

	ps("Activating final paging\n");
	err = paging_init();
	if (err)
		goto out;

	err = fdt_init(__fdt);
	if (err)
		goto out;

	err = mm_init_late();
	if (err)
		goto out;

	err = platform_init();
	if (err)
		goto out;

	err = sbi_init();
	if (err)
		goto out;

	ps("Disabling IRQs\n");
	irq_disable();

	/* Initialise external interrupts */
	ps("Initialising PLIC...\n");
	err = plic_init();
	if (err)
		goto out;

	/* enable external IRQs */
	ext_enable();

	ps("Initialising Serial...\n");
	err = serial_init();
	if (err)
		goto out;
	ps("Switched over from SBI to UART\n");

	/* Boot secondary CPUs */
	ps("Booting secondary CPUs\n");
	err = smp_init();
	if (err)
		goto out;

out:
	pr("End reached: %d\n", err);
}
