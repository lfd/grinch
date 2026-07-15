/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2025
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/gfp.h>
#include <grinch/task.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>

void arch_kinfo_init(struct kinfo *kinfo)
{
}

void task_set_context(struct task *task, unsigned long pc, unsigned long sp)
{
	task->regs.pc = pc;
	task->regs.sp = sp;
}

void arch_process_activate(struct process *p)
{
	unsigned long tmp;

	arm_write_sysreg(TTBR0, v2p(p->mm.page_table) |
			 p->mm.asid << TTBR_ASID_SHIFT);
	instruction_barrier();

	/*
	 * A process without its own ASID shares ASID 0 with every other
	 * untagged process, so its predecessor's user translations must be
	 * flushed here - there is no tag to keep them apart.
	 */
	if (!p->mm.asid)
		local_flush_tlb_asid(0);

	/* The kernel runs with TTBR0 walks off; enable them for EL0 */
	arm_read_sysreg(TCR, tmp);
	tmp &= ~TCR_EL1_EPD0;
	arm_write_sysreg(TCR, tmp);

	/* We want to return to EL0 */
	arm_read_sysreg(SPSR_EL1, tmp);
	tmp &= ~0xf;
	arm_write_sysreg(SPSR_EL1, tmp);
}

/*
 * On arm64 the kernel lives in TTBR1 and is shared by every address
 * space; only user mappings sit in mm->page_table (TTBR0). A freshly
 * created address space therefore needs nothing installed into it.
 */
void arch_mm_init(struct mm *mm)
{
}

/* Disable TTBR0 walks so a freed process root is never walked. */
void arch_process_deactivate(void)
{
	unsigned long tmp;

	arm_read_sysreg(TCR, tmp);
	tmp |= TCR_EL1_EPD0;
	arm_write_sysreg(TCR, tmp);
	instruction_barrier();
}
