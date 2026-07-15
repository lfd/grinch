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

#include <asm/sysregs.h>

#include <grinch/irqchip.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/percpu.h>
#include <grinch/syscall.h>
#include <grinch/task.h>

#include <grinch/cpu.h>

void arch_handle_exception(struct registers *regs);
void arch_handle_irq(struct registers *regs);

static void handle_dabt(struct trap_context *ctx)
{
	unsigned long far;
	bool is_write;
	u32 iss;

	iss = ESR_ISS(ctx->esr);
	is_write = iss & ESR_ISS_WNR;

	arm_read_sysreg(FAR_EL1, far);
	task_handle_fault((void __user *)far, is_write);
}

static int handle_user_abort(struct trap_context *ctx)
{
	struct registers *regs;
	int err;

	regs = &current_task()->regs;

	switch (ESR_EC(ctx->esr)) {
		case ESR_EC_SVC64:
			struct syscall_args args;
			args.arg1 = regs->usr[0];
			args.arg2 = regs->usr[1];
			args.arg3 = regs->usr[2];
			args.arg4 = regs->usr[3];
			args.arg5 = regs->usr[4];
			args.arg6 = regs->usr[5];
			syscall(regs->usr[8], &args);
			err = 0;
			break;

		case ESR_EC_DABT_LOW:
			handle_dabt(ctx);
			err = 0;
			break;

		default:
			err = -ENOSYS;
			break;
	}

	return err;
}

void arch_handle_exception(struct registers *regs)
{
	struct trap_context ctx;
	int err;

	arm_read_sysreg(esr_el1, ctx.esr);

	arm_read_sysreg(spsr_el1, regs->spsr);
	arm_read_sysreg(elr_el1, regs->pc);
	arm_read_sysreg(elr_el1, regs->elr);

	err = -1;
	switch (SPSR_EL(regs->spsr)) {
		case 0:
			arm_read_sysreg(SP_EL0, regs->sp);
			current_task()->regs = *regs;
			err = handle_user_abort(&ctx);
			break;

		case 1:
			/*
			 * We cannot access sp_el1 in EL1, so we have to calculate sp via regs.
			 */
			regs->sp = (unsigned long)regs + sizeof(struct registers);
			break;

		default:
			pr("Exception taken from unknown EL!\n");
			break;
	}

	if (err) {
		pr("\nFATAL synchronous exception on CPU %lu from EL%ld\n",
		   this_cpu_id(), SPSR_EL(regs->spsr));

		dump_exception(&ctx);
		dump_regs(regs);
		panic("System halted\n");
	}

	prepare_user_return();
}

/*
 * We reschedule (preempt) only when the interrupt was taken from EL0.
 * Only then is the register frame the canonical top-of-stack slot
 * (this_per_cpu()->stack.regs) that task_save()/task_restore() and
 * return_to_user (head.S) all agree on, so schedule() may swap it.
 *
 * An IRQ taken at EL1 -- e.g. while idling in do_idle(), which runs
 * wfi with IRQs enabled -- nests its frame lower on the stack, so we must
 * not reschedule here; prepare_user_return()'s retry loop picks up the new
 * task once we unwind back to idle. RISC-V's arch_handle_irq works the
 * same way.
 */
void arch_handle_irq(struct registers *regs)
{
	unsigned long spsr;

	arm_read_sysreg(spsr_el1, spsr);
	arm_read_sysreg(elr_el1, regs->pc);
	regs->spsr = spsr;

	if (SPSR_EL(spsr) == 0)
		arm_read_sysreg(SP_EL0, regs->sp);

	if (irqchip_fn)
		irqchip_fn->handle_irq();
	else
		pr("\nFATAL IRQ on CPU %lu: No IRQ driver\n", this_cpu_id());

	if (SPSR_EL(spsr) == 0 && !this_per_cpu()->idling) {
		task_save(regs);
		/*
		 * prepare_user_return() -> task_restore() rewrites the frame
		 * (== regs on an EL0 entry) with the scheduled task's context,
		 * which return_to_user then erets to -- no further copy needed.
		 */
		prepare_user_return();
	}
}
