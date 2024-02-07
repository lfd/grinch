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
#include <grinch/paging.h>
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


static int handle_syscall(void)
{
	struct registers *regs;
	int err;

	/* we had an ecall, so skip 4b of instructions */
	regs = &current_task()->regs;
	regs->pc += 4;

	err = syscall(regs->a7, regs->a0, regs->a1, regs->a2,
		      regs->a3, regs->a4, regs->a5);

	return err;
}

void arch_handle_irq(struct registers *regs, u64 scause)
{
	int err;
	u64 irq;
	bool prepare_user = false;

	if (!this_per_cpu()->idling)
		task_save(regs);

	irq = to_irq(scause);
	switch (irq) {
		case IRQ_S_SOFT:
			err = 0;
			this_per_cpu()->schedule = true;
			/* IPIs need to be acknowledged */
			ipi_clear();
			prepare_user = true;
			break;

		case IRQ_S_TIMER:
			err = arch_handle_timer();
			prepare_user = true;
			break;

		case IRQ_S_EXT:
			err = irqchip_fn->handle_irq();
			break;

		default:
			printk("No Handler for IRQ %llu\n", irq);
			err = -EINVAL;
			break;
	}

	if (err)
		panic("Error handling IRQ!\n");

	if (prepare_user && !this_per_cpu()->idling)
		prepare_user_return();
}

void arch_handle_exception(struct registers *regs, u64 scause)
{
	const char *cause_str = "UNKNOWN";
	enum vmm_trap_result vmtr;
	struct trap_context ctx;
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
	switch (ctx.scause) {
		case EXC_INST_ACCESS:
		case EXC_LOAD_ACCESS:
		case EXC_STORE_ACCESS:
		case EXC_INST_PAGE_FAULT:
		case EXC_LOAD_PAGE_FAULT:
		case EXC_STORE_PAGE_FAULT:
		case EXC_INST_MISALIGNED:
		case EXC_LOAD_ACCESS_MISALIGNED:
		case EXC_AMO_ADDRESS_MISALIGNED:
			printk("Faulting Address: %016lx\n", csr_read(stval));
			break;

		case EXC_SYSCALL:
			err = handle_syscall();
			break;

		case EXC_BREAKPOINT:
			printk("BP occured @ PC: %016lx - Ignoring\n", regs->pc);
			err = -1;
			break;

		default:
			break;
	}

out:
	if (err) {
		if (ctx.scause <= 23)
			cause_str = causes[ctx.scause];
		pr("FATAL: Exception on CPU %lu. Cause: %lu (%s)\n",
		   this_cpu_id(), to_irq(ctx.scause), cause_str);
		if (!(ctx.sstatus & SR_SPP))
			pr("PID: %u\n", current_task()->pid);
		dump_regs(regs);
		panic_stop();
	}

	if (vmtr == VMM_HANDLED || !(ctx.sstatus & SR_SPP))
		prepare_user_return();
}
