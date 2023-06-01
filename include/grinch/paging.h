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

#ifndef _PAGING_H
#define _PAGING_H

#include <grinch/types.h>
#include <grinch/csr.h>

#define PMASK(X)	(~((X) - 1))

#define PAGE_SHIFT		12
#define PAGE_SIZE		_BITUL(PAGE_SHIFT)
#define PAGE_MASK		PMASK(PAGE_SIZE)
#define PAGE_OFFS_MASK		(~PAGE_MASK)

#define MEGA_PAGE_SHIFT 	(PAGE_SHIFT + VPN_SHIFT)
#define MEGA_PAGE_SIZE		_BITUL(MEGA_PAGE_SHIFT)
#define MEGA_PAGE_MASK		PMASK(MEGA_PAGE_SIZE)

#define GIGA_PAGE_SHIFT 	(PAGE_SHIFT + 2 * VPN_SHIFT)
#define GIGA_PAGE_SIZE		_BITUL(GIGA_PAGE_SHIFT)
#define GIGA_PAGE_MASK		PMASK(GIGA_PAGE_SIZE)

#define PAGES(X)		((X) / PAGE_SIZE)
#define MEGA_PAGES(X)		((X) / MEGA_PAGE_SIZE)

#define VPN_SHIFT	9
#define VPN_MASK	((1UL << VPN_SHIFT) - 1)
#define PTES_PER_PT	(PAGE_SIZE / sizeof(u64))

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

#define PAGE_FLAGS_MEM_RX	(RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(R) | RISCV_PTE_FLAG(X))
#define PAGE_FLAGS_MEM_RW	(RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(R) | RISCV_PTE_FLAG(W))
#define PAGE_FLAGS_MEM_RWXU	(RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(R) | RISCV_PTE_FLAG(W) | RISCV_PTE_FLAG(X) | RISCV_PTE_FLAG(U))
#define PAGE_FLAGS_MEM_RO	(RISCV_PTE_FLAG(V) | RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(R))
#define PAGE_FLAGS_DEVICE	PAGE_FLAGS_MEM_RW

#define PAGE_PRESENT_FLAGS	(RISCV_PTE_FLAG(G) | RISCV_PTE_FLAG(V))

#define INVALID_PHYS_ADDR	(~0UL)

#ifndef __ASSEMBLY__

/*
 * The number of pages that need to be reserved for the bootstrap page tables
 */
#define PIE_PAGES	5

typedef unsigned long *pt_entry_t;
typedef pt_entry_t page_table_t;

extern unsigned long satp_mode;

struct paging {
        /** Page size of terminal entries in this level or 0 if none are
         * supported. */
        unsigned long page_size;

        pt_entry_t (*get_entry)(page_table_t page_table, unsigned long virt);
        bool (*entry_valid)(pt_entry_t pte, unsigned long flags);
        void (*set_terminal)(pt_entry_t pte, unsigned long phys,
                             unsigned long flags);
        unsigned long (*get_phys)(pt_entry_t pte, unsigned long virt);
        unsigned long (*get_flags)(pt_entry_t pte);
        void (*set_next_pt)(pt_entry_t pte, unsigned long next_pt);
        unsigned long (*get_next_pt)(pt_entry_t pte);
        void (*clear_entry)(pt_entry_t pte);
        bool (*page_table_empty)(page_table_t page_table);
};

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

static inline u64 page_up(u64 diff)
{
	return (diff + PAGE_SIZE - 1) & PAGE_MASK;
}

static inline u64 mega_page_up(u64 diff)
{
	return (diff + MEGA_PAGE_SIZE - 1) & MEGA_PAGE_MASK;
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

int paging_init(void);
int paging_cpu_init(unsigned long hart_id);

/* Versatile mapper */
int map_range(page_table_t pt, const void *vaddr, paddr_t paddr, size_t size,
	      u16 flags);
int unmap_range(page_table_t pt, const void *vaddr, size_t size);

#endif /* __ASSEMBLY__ */

#endif /* _PAGING_H */
