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

void arch_task_restore(void)
{
	struct task *task = current_task();

	this_per_cpu()->stack.regs = task->regs;

	csr_write(sepc, task->regs.sepc);
	csr_write(sscratch, task->regs.sp);
}

void arch_process_activate(struct process *process)
{
	struct per_cpu *tpcpu;

	tpcpu = this_per_cpu();
	memcpy(tpcpu->root_table_page, process->mm.page_table, PAGE_SIZE / 2);
	/* Let's make our life easy */
	// FIXME: think again about precise cache flushes
	flush_tlb_all();
	asm volatile("fence.i");
}
