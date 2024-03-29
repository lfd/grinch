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
#include <grinch/init.h>

#define for_each_cpu_except(cpu, set, exception)                \
	for ((cpu) = -1;                                        \
	     (cpu) = next_cpu((cpu), (set), (exception)),       \
	     (cpu) <= (MAX_CPUS - 1);	                        \
	    )
#define for_each_cpu(cpu, set)		for_each_cpu_except(cpu, set, -1)

typedef void (*smp_call_func_t)(void *info);

#include <grinch/percpu.h>

extern unsigned long cpus_available[BITMAP_ELEMS(MAX_CPUS)];
#define for_each_available_cpu_except(cpu, exception)	\
	for_each_cpu_except(cpu, cpus_available, exception)

#define for_each_available_cpu(cpu)			\
	for_each_available_cpu_except(cpu, -1)

#define for_each_available_cpu_except_this(cpu)		\
	for_each_available_cpu_except(cpu, this_cpu_id())

extern unsigned long cpus_online[BITMAP_ELEMS(MAX_CPUS)];
#define for_each_online_cpu_except(cpu, exception)	\
	for_each_cpu_except(cpu, cpus_online, exception)

#define for_each_online_cpu(cpu)			\
	for_each_online_cpu_except(cpu, -1)

#define for_each_online_cpu_except_this(cpu)		\
	for_each_online_cpu_except(cpu, this_cpu_id())

unsigned int next_cpu(unsigned int cpu, unsigned long *bitmap,
		      unsigned int exception);

int arch_boot_cpu(unsigned long cpu);

int smp_init(void);

void ipi_send(unsigned long cpu_id);
void ipi_broadcast(void);

void check_events(void);
void arch_do_idle(void);

void on_each_cpu(smp_call_func_t func, void *info);

#endif /* _SMP_H */
