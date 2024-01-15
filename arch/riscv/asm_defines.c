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

#include <asm_generic/asm_defines.h>
#include <grinch/percpu.h>

void common(void)
{
	DEFINE(STRUCT_REGISTERS_SIZE, sizeof(struct registers));
	DEFINE(STRUCT_PER_CPU_SIZE, sizeof(struct per_cpu));

	OFFSET(REG_SP, registers, sp);
	OFFSET(REG_PC, registers, pc);

	DEFINE(STACK_TOP,
		PERCPU_BASE + __builtin_offsetof(struct per_cpu, stack) + STACK_SIZE);
}
