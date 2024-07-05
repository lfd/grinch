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

#include <asm/spinlock.h>

#include <grinch/cpu.h>
#include <grinch/hypercall.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/stackdump.h>

#define PANIC_PREFIX	"P A N I C: "

static bool is_panic;
static DEFINE_SPINLOCK(panic_lock);

static void __noreturn panic_stop(void)
{
	spin_lock(&panic_lock);
	pr(PANIC_PREFIX "CPU %lu HALTED\n", this_cpu_id());
	spin_unlock(&panic_lock);
	if (grinch_is_guest)
		hypercall_vmquit(-1);
	cpu_halt();
}

void check_panic(void)
{
	if (is_panic)
		panic_stop();
}

void __noreturn __printf(1, 2) _panic(const char *fmt, ...)
{
	va_list ap;

	spin_lock(&panic_lock);
	is_panic = true;
	mb();
	ipi_broadcast();

	va_start(ap, fmt);
	vprintk(fmt, PANIC_PREFIX, ap);
	va_end(ap);

	stackdump();
	spin_unlock(&panic_lock);

	panic_stop();
}
