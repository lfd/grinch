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

#ifndef _CPU_H
#define _CPU_H

#ifndef __ASSEMBLY__

#include <asm/irq.h>

#include <grinch/compiler_attributes.h>
#include <grinch/types.h>

extern const char *causes[];

struct registers {
	unsigned long ra;
	unsigned long sp;
	unsigned long gp;
	unsigned long tp;
	unsigned long t0;
	unsigned long t1;
	unsigned long t2;
	unsigned long s0;
	unsigned long s1;
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
	unsigned long s2;
	unsigned long s3;
	unsigned long s4;
	unsigned long s5;
	unsigned long s6;
	unsigned long s7;
	unsigned long s8;
	unsigned long s9;
	unsigned long s10;
	unsigned long s11;
	unsigned long t3;
	unsigned long t4;
	unsigned long t5;
	unsigned long t6;

	unsigned long pc;
} __packed;

struct trap_context {
	/* Privileged registers */
	unsigned long scause;
	unsigned long sstatus;

	/* Hypervisor registers */
	unsigned long hstatus;
};

static inline void regs_set_retval(struct registers *r, unsigned long val)
{
	r->a0 = val;
}

static __always_inline void cpu_relax(void)
{
	asm volatile ("" : : : "memory");
}

static __always_inline void wait_for_interrupt(void)
{
	asm volatile("wfi\n" : : : "memory");
}

static __always_inline void mb(void)
{
	asm volatile("fence iorw, iorw\n" : : : "memory");
}

static inline void cpu_do_idle(void)
{
	mb();
	wait_for_interrupt();
}

static __always_inline void local_hfence_vvma_all(void)
{
	asm volatile(".insn 0x22000073"); /* hfence.vvma zero, zero */
}

static __always_inline void local_flush_tlb_all(void)
{
	asm volatile("sfence.vma" : : : "memory");
}

static __always_inline void local_flush_tlb_page(paddr_t page_addr)
{
	asm volatile("sfence.vma /* rd, */ zero, %[addr]"
		     : : [addr] "r" (page_addr));
}

void flush_tlb_all(void);

static __always_inline void __noreturn cpu_halt(void)
{
	irq_disable();
	ipi_disable();
	timer_disable();
	ext_disable();
	asm volatile("j _cpu_halt");
	__builtin_unreachable();
}

void dump_regs(struct registers *a);
void dump_exception(struct trap_context *ctx);

void guest_init(void);

extern bool grinch_is_guest;

#endif /* __ASSEMBLY__ */

#endif /* _CPU_H */
