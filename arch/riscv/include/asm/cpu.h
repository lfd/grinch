/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _CPU_H
#define _CPU_H

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

	unsigned long sepc;
} __attribute__((packed));

struct trap_context {
	/* Privileged registers */
	unsigned long scause;
	unsigned long sstatus;
};

static inline void regs_set_retval(struct registers *r, unsigned long val)
{
	r->a0 = val;
}

static inline void cpu_relax(void)
{
	asm volatile ("" : : : "memory");
}

static inline void flush_tlb_all(void)
{
	asm volatile("sfence.vma" : : : "memory");
}

static inline void flush_tlb_page(paddr_t page_addr)
{
	asm volatile("sfence.vma /* rd, */ zero, %[addr]"
		     : : [addr] "r" (page_addr));
}

static inline void __attribute__((noreturn, always_inline)) cpu_halt(void)
{
	asm volatile("j _cpu_halt");
	__builtin_unreachable();
}

void dump_regs(struct registers *a);

void guest_init(void);

extern bool grinch_is_guest;

#endif /* _CPU_H */
