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

#include <grinch/errno.h>
#include <grinch/sbi.h>
#include <grinch/boot.h>
#include <grinch/hypercall.h>
#include <grinch/paging.h>
#include <grinch/printk.h>

bool grinch_is_guest;

const char *causes[] = {
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
	u64 satp;

	satp = csr_read(CSR_SATP);

	printk("SATP -- Mode: %llu, PFN: 0x%016llx\n"
	       " PC: 0x%016lx RA: 0x%016lx  SP: 0x%016lx\n"
	       " GP: 0x%016lx TP: 0x%016lx  T0: 0x%016lx\n"
	       " T1: 0x%016lx T2: 0x%016lx  S0: 0x%016lx\n"
	       " S1: 0x%016lx A0: 0x%016lx  A1: 0x%016lx\n"
	       " A2: 0x%016lx A3: 0x%016lx  A4: 0x%016lx\n"
	       " A5: 0x%016lx A6: 0x%016lx  A7: 0x%016lx\n"
	       " S2: 0x%016lx S3: 0x%016lx  S4: 0x%016lx\n"
	       " S5: 0x%016lx S6: 0x%016lx  S7: 0x%016lx\n"
	       " S8: 0x%016lx S9: 0x%016lx S10: 0x%016lx\n"
	       "S11: 0x%016lx T3: 0x%016lx  T4: 0x%016lx\n"
	       " T5: 0x%016lx T6: 0x%016lx\n",
			satp >> 60, (satp & SATP_PPN) << PAGE_SHIFT,
			a->sepc, a->ra, a->sp,
			a->gp, a->tp, a->t0,
			a->t1, a->t2, a->s0,
			a->s1, a->a0, a->a1,
			a->a2, a->a3, a->a4,
			a->a5, a->a6, a->a7,
			a->s2, a->s3, a->s4,
			a->s5, a->s6, a->s7,
			a->s8, a->s9, a->s10,
			a->s11, a->t3, a->t4,
			a->t5, a->t6);
}

int hypercall(unsigned long no, unsigned long arg1)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_GRNC, no, arg1, 0, 0, 0, 0, 0);
	if (ret.error != SBI_SUCCESS)
		return -EINVAL;

	return ret.value;
}

void guest_init(void)
{
	int ret;

	ret = hypercall_present();
	if (ret <= 0)
		return;

	grinch_is_guest = true;
	grinch_id = ret;
}
