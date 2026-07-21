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

/*
 * Install the kernel's root-level entries into a freshly created
 * address space. The kernel's root level is fixed once boot completes,
 * and all kernel mappings below it live in tables shared through these
 * entries, so this copy is done once and never needs refreshing. The
 * fence publishes the new entries to the page-table walker.
 */
void arch_mm_init(struct mm *mm)
{
	page_table_t pt = mm->page_table;
	unsigned int kernel_index;

#if CONFIG_ARCH_RISCV == 64 /* rv64 */
	/* On SV39, SV48, …: The upper half belongs to the kernel */
	kernel_index = PTES_PER_PT / 2;
#elif CONFIG_ARCH_RISCV == 32 /* rv32 */
	kernel_index = vaddr2vpn((void *)USER_END, 1);
#endif
	memcpy(&pt[kernel_index], &kernel_root[kernel_index],
	       (PTES_PER_PT - kernel_index) * sizeof(*pt));

	local_flush_tlb_all();
}

void arch_process_activate(struct process *process)
{
	/* Deactivate VMM */
	if (has_hypervisor())
		csr_write(CSR_HSTATUS, 0);

	/* Ensure that sret returns to U-Mode */
	csr_clear(sstatus, SR_SPP);

	switch_mmu_satp(process->mm.asid, v2p(process->mm.page_table));

	/*
	 * A process without its own ASID shares ASID 0 with every other
	 * untagged process, so its predecessor's user translations must be
	 * flushed here - there is no tag to keep them apart.
	 */
	if (!process->mm.asid)
		local_flush_tlb_all();

	asm volatile("fence.i");
}

void arch_process_deactivate(void)
{
	/* The kernel root holds only global entries: it lives under ASID 0. */
	switch_mmu_satp(0, v2p(kernel_root));
}
