/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/isa.h>

#include <grinch/gfp.h>
#include <grinch/task.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
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
	page_table_t pt;
	unsigned int kernel_index;

	/* Deactivate VMM */
	if (has_hypervisor())
		csr_write(CSR_HSTATUS, 0);

	/* Ensure that sret returns to U-Mode */
	csr_clear(sstatus, SR_SPP);

	/*
	 * Install the kernel entries, then make the process' page table
	 * the translation root. Kernel mappings share their lower-level
	 * tables across all roots, only the root level has to be kept
	 * up to date.
	 */
	pt = process->mm.page_table;
#if CONFIG_ARCH_RISCV == 64 /* rv64 */
	/* On SV39, SV48, …: The upper half belongs to the kernel */
	kernel_index = PTES_PER_PT / 2;
#elif CONFIG_ARCH_RISCV == 32 /* rv32 */
	kernel_index = vaddr2vpn((void *)USER_END, 1);
#endif
	memcpy(&pt[kernel_index], &this_root_table_page()[kernel_index],
	       (PTES_PER_PT - kernel_index) * sizeof(*pt));

	enable_mmu_satp(satp_mode, v2p(pt));
	asm volatile("fence.i");
}

void arch_process_deactivate(void)
{
	enable_mmu_satp(satp_mode, v2p(this_root_table_page()));
}
