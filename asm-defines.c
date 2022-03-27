/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/percpu.h>

#define DEFINE(sym, val) \
	asm volatile("\n=>" #sym " %0 " #val : : "i" (val))

#define BLANK() asm volatile("\n=>" : : )

#define OFFSET(sym, str, mem) \
	DEFINE(sym, __builtin_offsetof(struct str, mem))

#define COMMENT(x) \
	asm volatile("\n=>#" x)


void common(void);

void common(void)
{
	DEFINE(STRUCT_REGISTERS_SIZE, sizeof(struct registers));
	DEFINE(STRUCT_PER_CPU_SIZE, sizeof(struct per_cpu));

	DEFINE(STACK_TOP,
		PERCPU_BASE + __builtin_offsetof(struct per_cpu, stack) + STACK_SIZE);
	DEFINE(EXCEPTION_STACK_TOP,
		PERCPU_BASE + __builtin_offsetof(struct per_cpu, exception) + STACK_SIZE);

	OFFSET(OFF_PER_CPU_ROOT_TABLE, per_cpu, root_table_page);
}
