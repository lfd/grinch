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

#include <asm/irq.h>

#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/percpu.h>

bool is_panic;

void check_panic(void)
{
	if (is_panic)
		panic_stop();
}

void __noreturn panic_stop(void)
{
	irq_disable();
	ipi_disable();
	timer_disable();
	ext_disable();
	pr("Panic: CPU %lu HALTED\n", this_cpu_id());
	cpu_halt();
}
