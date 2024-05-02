/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/cpu.h>
#include <asm/irq.h>
#include <asm/isa.h>

#include <grinch/irqchip.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/percpu.h>
#include <grinch/symbols.h>
#include <grinch/syscall.h>
#include <grinch/task.h>
#include <grinch/timer.h>

#include <grinch/arch/sbi.h>

/* called from entry.S */
void arch_handle_exception(struct registers *regs, u64 scause);
void arch_handle_irq(struct registers *regs, u64 scause);

static void handle_syscall(void)
{
	struct registers *regs;

	/* we had an ecall, so skip 4b of instructions */
	regs = &current_task()->regs;
	regs->pc += 4;

	syscall(regs->a7, regs->a0, regs->a1, regs->a2, regs->a3, regs->a4,
		regs->a5);
}

void arch_handle_irq(struct registers *regs, u64 scause)
{
	bool prepare_user = false;
	u64 irq;

	if (!this_per_cpu()->idling)
		task_save(regs);
	else
		BUG();

	irq = to_irq(scause);
	switch (irq) {
		case IRQ_S_SOFT:
			ipi_clear();
			this_per_cpu()->handle_events = true;
			prepare_user = true;
			break;

		case IRQ_S_TIMER:
			handle_timer();
			this_per_cpu()->handle_events = true;
			prepare_user = true;
			break;

		case IRQ_S_EXT:
			irqchip_fn->handle_irq();
			break;

		default:
			panic("No Handler for IRQ %llu\n", irq);
			break;
	}

	if (prepare_user && !this_per_cpu()->idling)
		prepare_user_return();
}

void arch_handle_exception(struct registers *regs, u64 scause)
{
	enum vmm_trap_result vmtr;
	struct trap_context ctx;
	void __user *stval;
	int err;

	ctx.scause = scause;
	ctx.sstatus = csr_read(sstatus);

	vmtr = VMM_FORWARD;
	if (has_hypervisor()) {
		vmtr = vmm_handle_trap(&ctx, regs);
		if (vmtr == VMM_HANDLED) {
			err = 0;
			goto out;
		}
		if (vmtr == VMM_ERROR) {
			err = -EINVAL;
			goto out;
		}
	}

	err = -EINVAL;
	if (ctx.sstatus & SR_SPP) {
		pr("FATAL: Trap taken from Supervisor mode\n");
		goto out;
	}

	task_save(regs);
	stval = (void __user *)csr_read(stval);
	switch (ctx.scause) {
		case EXC_INST_ILLEGAL:
		case EXC_INST_PAGE_FAULT:
			dump_exception(&ctx);
			//dump_regs(regs);
			task_exit(current_task(), -EFAULT);
			err = 0;
			break;

		case EXC_LOAD_PAGE_FAULT:
			task_handle_fault(stval, false);
			err = 0;
			break;

		case EXC_STORE_PAGE_FAULT:
			task_handle_fault(stval, true);
			err = 0;
			break;

		case EXC_INST_ACCESS:
		case EXC_LOAD_ACCESS:
		case EXC_STORE_ACCESS:
		case EXC_INST_MISALIGNED:
		case EXC_LOAD_ACCESS_MISALIGNED:
		case EXC_AMO_ADDRESS_MISALIGNED:
			pr("Faulting Address: %p\n", stval);
			break;

		case EXC_SYSCALL:
			handle_syscall();
			err = 0;
			break;

		case EXC_BREAKPOINT:
			pr("BP occured @ PC: %016lx - Ignoring\n", regs->pc);
			err = -1;
			break;

		default:
			break;
	}

out:
	if (err) {
		dump_exception(&ctx);
		dump_regs(regs);
		panic("System halted\n");
	}

	if (vmtr == VMM_HANDLED || !(ctx.sstatus & SR_SPP))
		prepare_user_return();
}
