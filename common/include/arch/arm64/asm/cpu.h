/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ASM_CPU_H
#define _ASM_CPU_H

#include <asm/sysregs.h>

#include <asm/irq.h>

#include <grinch/types.h>
#include <grinch/paging.h>

#define REG_FMT		"%016lx"

#define NUM_REGS	31

#define dmb(domain)	asm volatile("dmb " #domain ::: "memory")
#define dsb(opt)	asm volatile("dsb " #opt : : : "memory")
#define isb()		asm volatile("isb" : : : "memory")

struct registers {
	/* Must be aligned to 64 bit */
	unsigned long usr[NUM_REGS];
	unsigned long __padding;

	/* Must be aligned to 64 bit. Do not move location. */
	unsigned long sp;
	unsigned long pc;

	unsigned long spsr;
	unsigned long elr;
} __attribute__((packed));

struct trap_context {
	unsigned long esr;
};

static inline void regs_set_retval(struct registers *r, unsigned long val)
{
	r->usr[0] = val;
}

static inline void memory_barrier(void)
{
	dmb(ish);
}

#define mb()		memory_barrier()

static inline void synchronization_barrier(void)
{
	dsb(ish);
}

static inline void instruction_barrier(void)
{
	asm volatile("isb");
}

static inline void cpu_relax(void)
{
	asm volatile ("" : : : "memory");
}

static inline void wfi(void)
{
	asm volatile("wfi" : : : "memory");
}

static inline void wait_for_interrupt(void)
{
	/* WFI needs prior memory accesses completed (DSB), not just ordered. */
	synchronization_barrier();
	wfi();
}

static inline void local_flush_tlb_all(void)
{
	dsb(ishst);
	asm volatile("tlbi vmalle1is\r\n");
	synchronization_barrier();
	isb();
}

static inline void flush_tlb_all(void)
{
	local_flush_tlb_all();
}

static inline void local_flush_tlb_guest_all(void)
{
	/* No stage-2 support on arm64 (yet) */
	for(;;); /* IMPLEMENT ME! */
}

/* Flush all of one address space on this CPU (ASID in bits [63:48]). */
static inline void local_flush_tlb_asid(unsigned long asid)
{
	asm volatile(
		"dsb ishst\n\t"
		"tlbi aside1, %0\n\t"
		"dsb ish\n\t"
		"isb\n\t"
		: : "r" (asid << 48) : "memory");
}

static inline void local_flush_tlb_page(paddr_t addr)
{
	/*
	 * vaae1is: invalidate the address in every address space, the
	 * counterpart of riscv's sfence.vma addr, x0. The invalidation
	 * broadcasts across the inner-shareable domain, which
	 * flush_tlb_others_asid() relies on. Bits [43:0] hold VA[55:12];
	 * everything above must stay clear.
	 */
	asm volatile(
		"dsb ishst\n\t"
		"tlbi vaae1is, %0\n\t"
		"dsb ish\n\t"
		"isb\n\t"
		: : "r" ((addr >> PAGE_SHIFT) & GENMASK_ULL(43, 0)) : "memory");
}

static inline void __attribute__((noreturn, always_inline)) cpu_halt(void)
{
	asm volatile("b _cpu_halt");
	__builtin_unreachable();
}

static inline u64 timer_get_frequency(void)
{
        u64 freq;

        arm_read_sysreg(CNTFRQ_EL0, freq);
        return freq;
}

static inline u64 timer_get_ticks(void)
{
        u64 pct64;

        arm_read_sysreg(CNTPCT_EL0, pct64);
        return pct64;
}

static inline void timer_start(u64 timeout)
{
	arm_write_sysreg(CNTV_TVAL_EL0, timeout);
	arm_write_sysreg(CNTV_CTL_EL0, 1);
}

static inline void arch_guest_init(void) {}

#endif /* _ASM_CPU_H */
