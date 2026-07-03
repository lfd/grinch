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

#define MAIR_ATTR_SHIFT(__n)	((__n) << 3)
#define MAIR_ATTR(__n, __attr)	((__attr) << MAIR_ATTR_SHIFT(__n))
#define MAIR_ATTR_WBRWA		0xff
#define MAIR_ATTR_DEVICE	0x00    /* nGnRnE */

#define MAIR_IDX_WBRAWA         0
#define MAIR_IDX_DEV            1

#define MAIR0_DEFAULT		(MAIR_ATTR(0, MAIR_ATTR_WBRWA) | \
				 MAIR_ATTR(1, MAIR_ATTR_DEVICE))

#define PTE_MEMATTR(val)        ((val) << 2)
#define PTE_ACCESS_FLAG         (0x1 << 10)
#define PTE_INNER_SHAREABLE     (0x3 << 8)


#define PTE_FLAG_VALID		(1 << 0)
#define PTE_FLAG_TERMINAL	(1 << 1)
#define PTE_TABLE_FLAGS		(PTE_FLAG_VALID | PTE_FLAG_TERMINAL)
#define PTE_BLOCK_FLAGS		(PTE_FLAG_VALID)

#define S1_PTE_ACCESS_RW	(0x0 << 7)
#define S1_PTE_ACCESS_RO	(0x1 << 7)
#define S1_PTE_ACCESS_EL0       (0x1 << 6)
#define S1_PTE_NG		(0x1 << 11)

#define S1_PTE_UXN		(1UL << 54)
#define S1_PTE_PXN		(1UL << 53)

#define S1_PTE_FLAG_NORMAL	PTE_MEMATTR(MAIR_IDX_WBRAWA)
#define S1_PTE_FLAG_DEVICE	PTE_MEMATTR(MAIR_IDX_DEV)

#define S1_DEFAULT_FLAGS        (PTE_FLAG_VALID | PTE_ACCESS_FLAG       \
				| PTE_INNER_SHAREABLE)

#define S1_MEM_FLAGS		(S1_DEFAULT_FLAGS | S1_PTE_FLAG_NORMAL)
#define S1_DEVICE_FLAGS		(S1_DEFAULT_FLAGS | S1_PTE_FLAG_DEVICE)

#define PAGE_PRESENT_FLAGS	PTE_FLAG_VALID

/* Common definitions for page table structure in long descriptor format */
#define L0_VADDR_MASK	GENMASK_ULL(47, 39)
#define L1_VADDR_MASK	GENMASK_ULL(38, 30)
#define L2_VADDR_MASK	GENMASK_ULL(29, 21)
#define L3_VADDR_MASK	GENMASK_ULL(20, 12)

#define L0_IDX(VADDR)	(((VADDR) & L0_VADDR_MASK) >> 39)
#define L1_IDX(VADDR)	(((VADDR) & L1_VADDR_MASK) >> 30)
#define L2_IDX(VADDR)	(((VADDR) & L2_VADDR_MASK) >> 21)
#define L3_IDX(VADDR)	(((VADDR) & L3_VADDR_MASK) >> 12)

#ifndef __ASSEMBLY__

static inline unsigned long arch_paging_access_flags(mem_flags_t flags)
{
	/* IMPLEMENT ME! */
	for (;;);
	__builtin_unreachable();
}

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_PAGING_H */
