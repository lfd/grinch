/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ASM_CPU_H
#define _ASM_CPU_H

#ifndef __ASSEMBLY__

#include <asm/irq.h>

#include <grinch/compiler_attributes.h>
#include <grinch/types.h>

#if CONFIG_ARCH_RISCV == 32 /* rv32 */
#define REG_FMT_PFX	"08"
#elif CONFIG_ARCH_RISCV == 64 /* rv64 */
#define REG_FMT_PFX	"016"
#else
#error "Unknown RISC-V Architecture"
#endif

#define REG_FMT		"%" REG_FMT_PFX "lx"

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

static __always_inline void wfi(void)
{
	asm volatile("wfi\n" : : : "memory");
}

static __always_inline void mb(void)
{
	asm volatile("fence iorw, iorw\n" : : : "memory");
}

static inline void wait_for_interrupt(void)
{
	mb();
	wfi();
}

static __always_inline void local_hfence_vvma_all(void)
{
	asm volatile(".insn 0x22000073" : : : "memory"); /* hfence.vvma zero, zero */
}

static __always_inline void local_hfence_gvma_all(void)
{
	asm volatile(".insn 0x62000073" : : : "memory"); /* hfence.gvma zero, zero */
}

/* Flush guest (stage-2) translations on this CPU */
static __always_inline void local_flush_tlb_guest_all(void)
{
	local_hfence_gvma_all();
}

static __always_inline void local_flush_tlb_all(void)
{
	asm volatile("sfence.vma" : : : "memory");
}

static __always_inline void local_flush_tlb_page(paddr_t page_addr)
{
	/* sfence.vma rs1, rs2: rs1 holds the virtual address, rs2 the
	 * ASID. rs2 = x0 flushes the page across all address spaces. */
	asm volatile("sfence.vma %[addr], zero"
		     : : [addr] "r" (page_addr) : "memory");
}

/* Flush all of one address space on this CPU (rs1 = x0: every address). */
static __always_inline void local_flush_tlb_asid(unsigned long asid)
{
	asm volatile("sfence.vma zero, %[asid]"
		     : : [asid] "r" (asid) : "memory");
}

static __always_inline void __noreturn cpu_halt(void)
{
	irq_disable();
	ipi_disable();
	timer_disable();
	ext_disable();
	asm volatile("j _cpu_halt");
	__builtin_unreachable();
}

void arch_guest_init(void);

void flush_tlb_all(void);

#endif /* __ASSEMBLY__ */

#endif /* _ASM_CPU_H */
