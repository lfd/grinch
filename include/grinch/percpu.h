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

#ifndef _PERCPU_H
#define _PERCPU_H

#include <asm-generic/paging.h>

#define STACK_PAGES	1
#define STACK_SIZE	(STACK_PAGES * PAGE_SIZE)
#define MAX_CPUS	64

#ifndef __ASSEMBLY__

#include <asm-generic/grinch_layout.h>

#include <asm/cpu.h>
#include <asm/percpu.h>
#include <asm/spinlock.h>

#include <grinch/symbols.h>
#include <grinch/smp.h>
#include <grinch/time_abi.h>

struct per_cpu {
	union {
		unsigned char stack[STACK_SIZE];
		struct {
			unsigned char __fill[STACK_SIZE - sizeof(struct registers)];
			struct registers regs;
		};
	} stack;

	unsigned long root_table_page[PTES_PER_PT] __aligned(PAGE_SIZE);

	ARCH_PER_CPU_FIELDS

	int cpuid;

	bool pt_needs_update;
	bool primary;
	bool schedule;
	bool idling;
	bool handle_events;

	struct {
		timeu_t next;
	} timer;

	struct {
		spinlock_t lock;
		bool active;

		smp_call_func_t func;
		void *info;
	} remote_call;

	struct task *current_task;
} __aligned(PAGE_SIZE);

static __always_inline struct per_cpu *this_per_cpu(void)
{
	return (struct per_cpu*)PERCPU_BASE;
}

static __always_inline unsigned long this_cpu_id(void)
{
	return this_per_cpu()->cpuid;
}

static __always_inline struct per_cpu *per_cpu(unsigned long cpuid)
{
	return (struct per_cpu*)VMGRINCH_END - (cpuid + 1);
}

static __always_inline page_table_t this_root_table_page(void)
{
	return this_per_cpu()->root_table_page;
}

#endif /* __ASSEMBLY__ */

#endif
