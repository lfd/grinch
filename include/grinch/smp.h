/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
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
#include <grinch/paging.h>

#define for_each_cpu_except(cpu, set, exception)                \
	for ((cpu) = -1;                                        \
	     (cpu) = next_cpu((cpu), (set), (exception)),       \
	     (cpu) <= (MAX_CPUS - 1);	                        \
	    )
#define for_each_cpu(cpu, set)		for_each_cpu_except(cpu, set, -1)

typedef void (*smp_call_func_t)(void *info);

#include <grinch/percpu.h>

#define DEFINE_CPU_PREDICATE(name)				\
static inline bool cpu_is_##name(unsigned long cpu)		\
{								\
	return cpu < MAX_CPUS && test_bit(cpu, cpus_##name);	\
}								\
static inline void cpu_set_##name(unsigned long cpu)		\
{								\
	bitmap_set(cpus_##name, cpu, 1);			\
}

extern DECLARE_BITMAP(cpus_available, MAX_CPUS);
/* bool cpu_is_available(cpu); void cpu_set_available(cpu) */
DEFINE_CPU_PREDICATE(available)

#define for_each_available_cpu_except(cpu, exception)	\
	for_each_cpu_except(cpu, cpus_available, exception)

#define for_each_available_cpu(cpu)			\
	for_each_available_cpu_except(cpu, -1)

#define for_each_available_cpu_except_this(cpu)		\
	for_each_available_cpu_except(cpu, this_cpu_id())

extern DECLARE_BITMAP(cpus_online, MAX_CPUS);
/* bool cpu_is_online(cpu); void cpu_set_online(cpu) */
DEFINE_CPU_PREDICATE(online)

#define for_each_online_cpu_except(cpu, exception)	\
	for_each_cpu_except(cpu, cpus_online, exception)

#define for_each_online_cpu(cpu)			\
	for_each_online_cpu_except(cpu, -1)

#define for_each_online_cpu_except_this(cpu)		\
	for_each_online_cpu_except(cpu, this_cpu_id())

unsigned int next_cpu(unsigned int cpu, unsigned long *bitmap,
		      unsigned int exception);

int arch_boot_cpu(unsigned long cpu);

/* Arch hook: per-CPU C-side bring-up before the common secondary path */
void arch_secondary_init(void);

/* Arch hook: populate the arch view of secondary_boot_root before mapping */
void arch_smp_bringup_init(void);

/* Trampoline root the secondaries boot on (owned by kernel/smp.c). */
extern page_table_t secondary_boot_root;

int smp_init(void);

void ipi_send(unsigned long cpu_id);
void ipi_broadcast(void);

void check_events(void);

void on_each_cpu(smp_call_func_t func, void *info);

#endif /* _SMP_H */
