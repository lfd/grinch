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

#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/percpu.h>

void __attribute__((noreturn)) panic_stop(void)
{
	pr("Panic: CPU %lu HALTED\n", this_cpu_id());
	cpu_halt();
}
