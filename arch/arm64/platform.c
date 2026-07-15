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

#define dbg_fmt(x)	"platform: " x

#include <grinch/gfp.h>
#include <grinch/init.h>
#include <grinch/percpu.h>
#include <grinch/platform.h>
#include <grinch/printk.h>
#include <grinch/smp.h>

int __init arch_platform_init(void)
{
	unsigned long cpu;

	// FIXME: Determine available CPUs from FDT
	bitmap_set(cpus_available, 0, 1);

	/* Free per_cpu pages of absent CPUs back to the pool */
	for (cpu = 1; cpu < MAX_CPUS; cpu++)
		free_pages(per_cpu(cpu), PAGES(sizeof(struct per_cpu)));

	return 0;
}
