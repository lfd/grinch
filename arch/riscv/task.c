/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/isa.h>

#include <grinch/task.h>
#include <grinch/paging.h>
#include <grinch/string.h>

void task_set_context(struct task *task, unsigned long pc, unsigned long sp)
{
	task->regs.pc = pc;
	task->regs.sp = sp;
}

void arch_kinfo_init(struct kinfo *kinfo)
{
	kinfo->riscv.timebase_frequency = riscv_timebase_frequency;
}

void arch_process_activate(struct process *process)
{
	struct per_cpu *tpcpu;
	size_t len;

	/* Deactivate VMM */
	if (has_hypervisor())
		csr_write(CSR_HSTATUS, 0);

	/* Ensure that sret returns to U-Mode */
	csr_clear(sstatus, SR_SPP);

	tpcpu = this_per_cpu();
#if ARCH_RISCV == 64 /* rv64 */
	/* On SV39, SV48, …: The lower half belongs to the user */
	len = PAGE_SIZE / 2;
#elif ARCH_RISCV == 32 /* rv32 */
	len = vaddr2vpn((void *)USER_END, 1) * sizeof(unsigned long);
#endif
	memcpy(tpcpu->root_table_page, process->mm.page_table, len);

	/* Let's make our life easy */
	// FIXME: think again about precise cache flushes
	local_flush_tlb_all();
	asm volatile("fence.i");
}
