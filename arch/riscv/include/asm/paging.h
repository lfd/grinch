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

#ifndef _ASM_PAGING_H
#define _ASM_PAGING_H

#include <asm/csr.h>

#define RISCV_PTE_V     0
#define RISCV_PTE_R     1
#define RISCV_PTE_W     2
#define RISCV_PTE_X     3
#define RISCV_PTE_U     4
#define RISCV_PTE_G     5
#define RISCV_PTE_A     6
#define RISCV_PTE_D     7
#define RISCV_PTE_RSW   8
#define RISCV_PTE_RSW_w 2

#define RISCV_PTE_BITS	10
#define RISCV_PTE_MASK	~((1ULL << RISCV_PTE_BITS) - 1)
#define RISCV_PTE_SHIFT	(PAGE_SHIFT - RISCV_PTE_BITS)

#define RISCV_PTE_FLAG(FLAG)    (1 << RISCV_PTE_ ## FLAG)

#define PAGE_FLAGS_DEFAULT	(0			\
				| RISCV_PTE_FLAG(G)	\
				| RISCV_PTE_FLAG(V)	\
				| RISCV_PTE_FLAG(R)	\
				| RISCV_PTE_FLAG(W)	\
				| RISCV_PTE_FLAG(X))

#define PAGE_FLAGS_MEM_RX		(RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(R) | RISCV_PTE_FLAG(X))
#define PAGE_FLAGS_MEM_RW		(RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(R) | RISCV_PTE_FLAG(W))
#define PAGE_FLAGS_MEM_RWXU		(RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(R) | RISCV_PTE_FLAG(W) | RISCV_PTE_FLAG(X) | RISCV_PTE_FLAG(U))
#define PAGE_FLAGS_MEM_RO		(RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(R))
/* FIXME */
#define PAGE_FLAGS_DEVICE		PAGE_FLAGS_MEM_RW

#define PAGE_PRESENT_FLAGS	(RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(V))

#ifndef __ASSEMBLY__

static inline unsigned int vaddr2vpn(const void *vaddr, unsigned char lvl)
{
	return ((uintptr_t)vaddr >> (PAGE_SHIFT + lvl * VPN_SHIFT)) & VPN_MASK;
}

static inline u64 paddr2pte(paddr_t paddr)
{
	return paddr >> RISCV_PTE_SHIFT;
}

static inline paddr_t pte2table(u64 pte)
{
	return (pte & RISCV_PTE_MASK) << RISCV_PTE_SHIFT;
}

#define ENABLE_MMU(NAME, REG)					\
static inline void enable_mmu_##NAME(u64 mode, paddr_t pt)	\
{								\
	u64 atp;						\
								\
	atp = mode | (u64)pt >> PAGE_SHIFT;			\
	asm volatile("sfence.vma\n"				\
		     "csrw %0, %1\n"				\
		     "sfence.vma\n"				\
		     : : "i"(REG), "rK"(atp) : "memory");	\
}

ENABLE_MMU(satp, CSR_SATP)
ENABLE_MMU(hgatp, CSR_HGATP)

extern unsigned long satp_mode;

#endif /* __ASSEMBLY__ */

#endif /* _ASM_PAGING_H */
