/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"smp: " x

#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/smp.h>

unsigned long available_cpus[BITMAP_ELEMS(MAX_CPUS)];

unsigned int next_cpu(unsigned int cpu, unsigned long *bitmap,
		      unsigned int exception)
{
	do
		cpu++;
	while (cpu <= MAX_CPUS &&
	       (cpu == exception || !test_bit(cpu, bitmap)));
	return cpu;
}

int __init smp_init(void)
{
	unsigned long cpu;
	int err;

	err = 0;
	for_each_cpu_except(cpu, available_cpus, this_cpu_id()) {
		err = arch_boot_cpu(cpu);
		if (err)
			return err;

		while (!per_cpu(cpu)->online)
			cpu_relax();
		pri("CPU %lu online!\n", cpu);
	}

	return err;
}
