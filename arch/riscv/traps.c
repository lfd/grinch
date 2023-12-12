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

#include <asm/cpu.h>
#include <asm/irq.h>
#include <asm/isa.h>

#include <grinch/irqchip.h>
#include <grinch/paging.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/percpu.h>
#include <grinch/sbi.h>
#include <grinch/symbols.h>
#include <grinch/syscall.h>
#include <grinch/task.h>

void arch_handle_trap(struct registers *regs);

int handle_irq(u64 irq)
{
	int err;

	switch (irq) {
		case IRQ_S_SOFT:
			err = handle_ipi();
			/* IPIs need to be acknowledged */
			csr_clear(sip, IE_SIE);
			break;

		case IRQ_S_TIMER:
			err = handle_timer();
			break;

		case IRQ_S_EXT:
			err = irqchip_fn->handle_irq();
			break;

		default:
			printk("No Handler for IRQ %llu\n", irq);
			err = -1;
			break;
	}

	return err;
}

static int handle_syscall(void)
{
	struct registers *regs;
	int err;

	/* we had an ecall, so skip 4b of instructions */
	regs = &current_task()->regs;
	regs->sepc += 4;

	err = syscall(regs->a7, regs->a0, regs->a1, regs->a2,
		      regs->a3, regs->a4, regs->a5);

	return err;
}

void arch_handle_trap(struct registers *regs)
{
	const char *cause_str = "UNKNOWN";
	enum vmm_trap_result vmtr;
	struct trap_context ctx;
	int err;

	regs->sepc = csr_read(sepc);
	ctx.scause = csr_read(scause);
	ctx.sstatus = csr_read(sstatus);

	if (has_hypervisor()) {
		vmtr = vmm_handle_trap(&ctx, regs);
		if (vmtr == VMM_HANDLED) {
			err = 0;
			goto out;
		}
		if (vmtr == VMM_ERROR) {
			err = -EINVAL;
			goto out;
		} else if (vmtr == VMM_FORWARD) {
		}

	}

	if (is_irq(ctx.scause)) {
		err = handle_irq(to_irq(ctx.scause));
		goto out;
	}

	err = -EINVAL;
	if (ctx.sstatus & SR_SPP) {
		printk("FATAL: Trap taken from Supervisor mode\n");
		goto out;
	}

	/* Save task context */
	current_task()->regs = *regs;
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
			printk("BP occured @ PC: %016lx - Ignoring\n", regs->sepc);
			err = -1;
			break;

		default:
			break;
	}

out:
	if (err) {
		/* We end up here in case of exceptions */
		if (ctx.scause <= 23)
			cause_str = causes[ctx.scause];
		else if (is_irq(ctx.scause))
			cause_str = "IRQ";

		pr("FATAL: Exception on CPU %lu. Cause: %lu (%s)\n",
		   this_cpu_id(), to_irq(ctx.scause), cause_str);
		if (!(ctx.sstatus & SR_SPP))
			pr("PID: %u\n", current_task()->pid);
		dump_regs(regs);
		panic_stop();
	}

	prepare_user_return();
}
