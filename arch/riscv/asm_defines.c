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

#include <asm-generic/asm_defines.h>
#include <grinch/percpu.h>
#include <grinch/task.h>

void common(void)
{
	OFFSET(TASK_THREAD_RA, task, thread.ra);
	OFFSET(TASK_THREAD_SP, task, thread.sp);
	OFFSET(TASK_THREAD_S0, task, thread.s);

	DEFINE(STRUCT_REGISTERS_SIZE, sizeof(struct registers));
	DEFINE(STRUCT_PER_CPU_SIZE, sizeof(struct per_cpu));
	DEFINE(PCPU_STACK_TOP,
	       __builtin_offsetof(struct per_cpu, stack) + STACK_SIZE);

	OFFSET(PCPU_KERNEL_SP, per_cpu, kernel_sp);
	OFFSET(PCPU_USER_SP, per_cpu, user_sp);

	OFFSET(REG_SP, registers, sp);
	OFFSET(REG_TP, registers, tp);
	OFFSET(REG_PC, registers, pc);
	OFFSET(REG_FROM_KERNEL, registers, from_kernel);
}
