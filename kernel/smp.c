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

unsigned long cpus_available[BITMAP_ELEMS(MAX_CPUS)];
unsigned long cpus_online[BITMAP_ELEMS(MAX_CPUS)];

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
	unsigned long cpu, cpus;
	int err;

	cpus = 1;
	for_each_available_cpu_except_this(cpu) {
		err = arch_boot_cpu(cpu);
		if (err) {
			pri("Unable to boot CPU %lu\n", cpu);
			continue;
		}

		while (!test_bit(cpu, cpus_online))
			cpu_relax();
		pri("CPU %lu online!\n", cpu);
		cpus++;
	}

	pri("Successfully brought up %lu CPUs\n", cpus);
	return 0;
}

void ipi_broadcast(void)
{
	unsigned long cpu;

	for_each_online_cpu_except_this(cpu)
		ipi_send(cpu);
}
