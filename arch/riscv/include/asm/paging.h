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

#ifndef _ASM_PAGING_H
#define _ASM_PAGING_H

#include <asm/csr.h>
#include <asm/cpu.h>

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

#define PAGE_PRESENT_FLAGS	(RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(V))

#ifndef __ASSEMBLY__

static inline unsigned long arch_paging_access_flags(mem_flags_t flags)
{
	unsigned long ret;

	ret = RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) |
	      ((flags & GRINCH_MEM_R) ? RISCV_PTE_FLAG(R) : 0) |
	      ((flags & GRINCH_MEM_W) ? RISCV_PTE_FLAG(W) : 0) |
	      ((flags & GRINCH_MEM_U) ? RISCV_PTE_FLAG(U) : 0) |
	      ((flags & GRINCH_MEM_X) ? RISCV_PTE_FLAG(X) : 0);

	/*
	 * For the moment, mark all pages as accessed and dirty to prevent page
	 * faults
	 */
	ret |= (RISCV_PTE_FLAG(A) | RISCV_PTE_FLAG(D));

	return ret;
}

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

#define ATP(MODE, PT)	((MODE) | (u64)(PT) >> PAGE_SHIFT)

static inline void enable_mmu_satp(u64 mode, paddr_t pt)
{
	local_flush_tlb_all();
	csr_write(satp, ATP(mode, pt));
	local_flush_tlb_all();
}

static inline void enable_mmu_hgatp(u64 mode, paddr_t pt)
{
	local_hfence_vvma_all();
	csr_write(CSR_HGATP, ATP(mode, pt));
	local_hfence_vvma_all();
}

static inline void disable_mmu_hgatp(void)
{
	local_hfence_vvma_all();
	csr_write(CSR_HGATP, 0);
	local_hfence_vvma_all();
}

extern unsigned long satp_mode;

#endif /* __ASSEMBLY__ */

#endif /* _ASM_PAGING_H */
