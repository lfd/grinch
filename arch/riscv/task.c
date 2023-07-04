/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/csr.h>

#include <grinch/task.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/paging.h>

void task_set_context(struct task *task, unsigned long pc, unsigned long sp)
{
	task->regs.sepc = pc;
	task->regs.sp = sp;
}

void arch_task_activate(struct task *task)
{
	struct per_cpu *tpcpu;

	tpcpu = this_per_cpu();
	memcpy(tpcpu->root_table_page, task->mm.page_table, PAGE_SIZE / 2);
	/* Let's make our life easy */
	flush_tlb_all();

	csr_write(sepc, task->regs.sepc);

	memcpy(&tpcpu->stack.regs, &task->regs, sizeof(task->regs));
}
