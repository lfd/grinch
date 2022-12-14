/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/cpu.h>
#include <grinch/irq.h>
#include <grinch/printk.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/plic.h>
#include <grinch/sbi.h>
#include <grinch/sbi_handler.h>
#include <grinch/symbols.h>

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
			err = plic_handle_irq();
			break;

		default:
			printk("No Handler for IRQ %llu\n", irq);
			err = -1;
			break;
	}

	return err;
}

void arch_handle_trap(struct registers *regs)
{
	const char *cause_str = "UNKNOWN";
	unsigned long cause;
	int err = -1;

	regs->sepc = csr_read(sepc);
	cause = csr_read(scause);

	if (is_irq(cause)) {
		err = handle_irq(to_irq(cause));
		goto out;
	}

	switch (cause) {
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

		case EXC_SUPERVISOR_SYSCALL:
			err = handle_ecall(regs);
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
		if (cause <= 23)
			cause_str = causes[cause];
		else if (is_irq(cause))
			cause_str = "IRQ";

		pr("FATAL Exception on CPU %lu. Cause: %lu (%s)\n",
		   this_cpu_id(), to_irq(cause), cause_str);
		dump_regs(regs);
		pr("CPU %lu HALTED\n", this_cpu_id());
		for (;;)
			cpu_relax();
	}

	csr_write(sepc, regs->sepc);
}
