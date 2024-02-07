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

#ifndef _SMP_H
#define _SMP_H

#include <grinch/bitmap.h>
#include <grinch/percpu.h>

#define for_each_cpu(cpu, set)  for_each_cpu_except(cpu, set, -1)
#define for_each_cpu_except(cpu, set, exception)                \
	for ((cpu) = -1;                                        \
	     (cpu) = next_cpu((cpu), (set), (exception)),       \
	     (cpu) <= (MAX_CPUS - 1);	                        \
	    )

extern unsigned long cpus_available[BITMAP_ELEMS(MAX_CPUS)];
extern unsigned long cpus_online[BITMAP_ELEMS(MAX_CPUS)];

unsigned int next_cpu(unsigned int cpu, unsigned long *bitmap,
		      unsigned int exception);

int arch_boot_cpu(unsigned long cpu);

int platform_init(void);
int smp_init(void);

#endif /* _SMP_H */
