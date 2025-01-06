/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2025
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/irq.h>

#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/boot.h>
#include <grinch/hypercall.h>
#include <grinch/irqchip.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/timer.h>

#include <grinch/arch/sbi.h>

bool grinch_is_guest;

static const char *causes[] = {
	[EXC_INST_MISALIGNED]		= "Instruction Address Misaligned",
	[EXC_INST_ACCESS]		= "Instruction Address Fault",
	[EXC_INST_ILLEGAL]		= "Illegal Instruction",
	[EXC_BREAKPOINT]		= "Breakpoint",
	[EXC_LOAD_ACCESS_MISALIGNED]	= "Load Accesss Misaligned",
	[EXC_LOAD_ACCESS]		= "Load Access Fault",
	[EXC_AMO_ADDRESS_MISALIGNED]	= "AMO Address Misaligned",
	[EXC_STORE_ACCESS]		= "Store Access Fault",
	[EXC_SYSCALL]			= "Env Call From U-Mode",
	[EXC_HYPERVISOR_SYSCALL]	= "Env Call From S-Mode",
	[EXC_SUPERVISOR_SYSCALL]	= "Env Call From VS-Mode",
	[11]				= "Env Call From M-Mode",
	[EXC_INST_PAGE_FAULT]		= "Instruction Pagefault",
	[EXC_LOAD_PAGE_FAULT]		= "Load Pagefault",
	[14]				= "Reserved",
	[EXC_STORE_PAGE_FAULT]		= "Store Pagefault",
	[16 ... 19]			= "Reserved",
	[EXC_INST_GUEST_PAGE_FAULT]	= "Inst Guest Pagefault",
	[EXC_LOAD_GUEST_PAGE_FAULT]	= "Load Guest Pagefault",
	[EXC_VIRTUAL_INST_FAULT]	= "Virtual Instruction Fault",
	[EXC_STORE_GUEST_PAGE_FAULT]	= "Store Guest Pagefault",
};

void dump_regs(struct registers *a)
{
	unsigned long satp;

	satp = csr_read(CSR_SATP);

	pr("SATP -- Mode: %lu, PFN: " REG_FMT "     STVAL: " REG_FMT "\n",
			satp >> SATP_MODE_SHIFT,
			(satp & SATP_PPN) << PAGE_SHIFT,
			csr_read(stval));
	pr(" PC: " REG_FMT " RA: " REG_FMT "  SP: " REG_FMT "\n",
	   a->pc, a->ra, a->sp);
	pr(" GP: " REG_FMT " TP: " REG_FMT "  T0: " REG_FMT "\n",
	   a->gp, a->tp, a->t0);
	pr(" T1: " REG_FMT " T2: " REG_FMT "  S0: " REG_FMT "\n",
	   a->t1, a->t2, a->s0);
	pr(" S1: " REG_FMT " A0: " REG_FMT "  A1: " REG_FMT "\n",
	   a->s1, a->a0, a->a1);
	pr(" A2: " REG_FMT " A3: " REG_FMT "  A4: " REG_FMT "\n",
	   a->a2, a->a3, a->a4);
	pr(" A5: " REG_FMT " A6: " REG_FMT "  A7: " REG_FMT "\n",
	   a->a5, a->a6, a->a7);
	pr(" S2: " REG_FMT " S3: " REG_FMT "  S4: " REG_FMT "\n",
	   a->s2, a->s3, a->s4);
	pr(" S5: " REG_FMT " S6: " REG_FMT "  S7: " REG_FMT "\n",
	   a->s5, a->s6, a->s7);
	pr(" S8: " REG_FMT " S9: " REG_FMT " S10: " REG_FMT "\n",
	   a->s8, a->s9, a->s10);
	pr("S11: " REG_FMT " T3: " REG_FMT "  T4: " REG_FMT "\n",
	   a->s11, a->t3, a->t4);
	pr(" T5: " REG_FMT " T6: " REG_FMT "\n", a->t5, a->t6);
}

void dump_exception(struct trap_context *ctx)
{
	const char *cause_str = "UNKNOWN";

	if (ctx->scause <= 23)
		cause_str = causes[ctx->scause];
	pr("FATAL: Exception on CPU %lu. Cause: %lu (%s)\n",
	   this_cpu_id(), to_irq(ctx->scause), cause_str);
	if (!(ctx->sstatus & SR_SPP))
		pr("Active PID: %u\n", current_task()->pid);
}

int hypercall(unsigned long no, unsigned long arg1)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_GRNC, no, arg1, 0, 0, 0, 0, 0);
	if (ret.error != SBI_SUCCESS)
		return -EINVAL;

	return ret.value;
}

void arch_do_idle(void)
{
	cpu_do_idle();

	check_panic();

	if (ipi_pending()) {
		ipi_clear();
		check_events();
	}

	if (timer_pending()) {
		handle_timer();
		this_per_cpu()->handle_events = true;
	}

	if (ext_pending())
		irqchip_fn->handle_irq();
}

void __init arch_guest_init(void)
{
	int ret;

	ret = hypercall_present();
	if (ret <= 0)
		return;

	grinch_is_guest = true;
	grinch_id = ret;
}

void flush_tlb_all(void)
{
	unsigned long hmask;
	struct sbiret ret;
	unsigned int cpu;

	local_flush_tlb_all();

	hmask = 0;
	for_each_online_cpu_except_this(cpu) {
		if (cpu > 63)
			BUG();
		hmask |= (1UL << cpu);
	}

	if (hmask) {
		ret = sbi_rfence_sfence_vma(hmask, 0, 0, 0);
		if (ret.error != SBI_SUCCESS)
			BUG();
	}
}
