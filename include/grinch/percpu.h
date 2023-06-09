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

#ifndef _PERCPU_H
#define _PERCPU_H

#include <asm_generic/grinch_layout.h>
#include <asm/percpu.h>

#include <grinch/paging.h>

#define STACK_PAGES	1
#define STACK_SIZE	(STACK_PAGES * PAGE_SIZE)
#define MAX_HARTS	64

#ifndef __ASSEMBLY__

#include <asm/cpu.h>

#include <grinch/symbols.h>

struct per_cpu {
	unsigned char stack[STACK_SIZE];
	unsigned long root_table_page[PTES_PER_PT]
		__attribute__((aligned(PAGE_SIZE)));

	ARCH_PER_CPU_FIELDS

	int cpuid;
} __attribute__((aligned(PAGE_SIZE)));

static inline struct per_cpu *this_per_cpu(void)
{
	return (struct per_cpu*)PERCPU_BASE;
}

static inline unsigned long this_cpu_id(void)
{
	return this_per_cpu()->cpuid;
}

static inline struct per_cpu *per_cpu(unsigned long cpuid)
{
	return (struct per_cpu*)__internal_page_pool_end - (cpuid + 1);
}

static inline page_table_t this_root_table_page(void)
{
	return this_per_cpu()->root_table_page;
}

#endif /* __ASSEMBLY__ */

#endif
