/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/paging.h>
#include <grinch/mmu.h>

#define	PAGE_BITS	12
#define	WORD_BITS	3 /* 1 << WORD_BITS == sizeof (void *) */
#define	MAX_FLAG	10
#define FLAG_MASK	((1 << MAX_FLAG) - 1)

#define	PAGE_LEVEL_BITS		(PAGE_BITS - WORD_BITS)
#define PAGE_LEVEL_MASK(ROOT)	((1 << (PAGE_LEVEL_BITS + 2 * !!(ROOT))) - 1)

#define	UNTRANSLATED_BITS(LEVEL) \
			((LEVEL) * PAGE_LEVEL_BITS + PAGE_BITS)

#define	PAGE_TERMINAL_FLAGS \
	(RISCV_PTE_FLAG(R) | RISCV_PTE_FLAG(W) | RISCV_PTE_FLAG(X))

static inline unsigned long pte2phys(unsigned long pte)
{
	return (pte & ~FLAG_MASK) << (PAGE_BITS - MAX_FLAG);
}

static inline unsigned long phys2pte(unsigned long phys)
{
	return phys >> (PAGE_BITS - MAX_FLAG);
}

#define DEF_GET_ENTRY(NAME, LEVEL, ROOT)			\
static pt_entry_t						\
sv## NAME ##_vpn## LEVEL ##_get_entry(page_table_t pt,		\
					unsigned long virt)	\
{								\
	return pt + ((virt >> UNTRANSLATED_BITS(LEVEL)) &	\
		    PAGE_LEVEL_MASK(ROOT));			\
}

DEF_GET_ENTRY(X, 0, false)
DEF_GET_ENTRY(X, 1, false)
DEF_GET_ENTRY(X, 2, false)
DEF_GET_ENTRY(X, 3, false)

static bool svX_entry_valid(pt_entry_t pte, unsigned long flags)
{
	return !!(*pte & RISCV_PTE_FLAG(V));
}

#define DEF_SET_TERMINAL(NAME, FLAGS)					\
static void sv## NAME ##_vpnX_set_terminal(pt_entry_t pte,		\
					   unsigned long phys,		\
					   unsigned long flags)		\
{									\
	*pte = FLAGS | flags | phys2pte(phys);				\
}

DEF_SET_TERMINAL(X, 0)

#define DEF_GET_PHYS(LEVEL)					\
static unsigned long						\
svX_vpn## LEVEL ##_get_phys (pt_entry_t pte, unsigned long virt)\
{								\
	unsigned long entry = *pte;				\
	if (!(RISCV_PTE_FLAG(V) & entry) ||			\
	    !(PAGE_TERMINAL_FLAGS & (entry)))			\
		return INVALID_PHYS_ADDR;			\
	return pte2phys(entry) |				\
	       (((1UL << UNTRANSLATED_BITS(LEVEL)) - 1) & virt);\
}

DEF_GET_PHYS(0)
DEF_GET_PHYS(1)
DEF_GET_PHYS(2)
DEF_GET_PHYS(3)

static unsigned long svX_get_flags(pt_entry_t pte)
{
	return *pte & FLAG_MASK;
}

#define DEF_SET_NEXT(NAME, FLAGS)					\
static void								\
sv## NAME ##_vpnX_set_next_pt(pt_entry_t pte, unsigned long next_pt)	\
{									\
	*pte = FLAGS | RISCV_PTE_FLAG(V) | phys2pte(next_pt);		\
}

DEF_SET_NEXT(X, RISCV_PTE_FLAG(G))

static unsigned long svX_vpnX_get_next_pt(pt_entry_t pte)
{
	return pte2phys(*pte);
}

static void svX_clear_entry(pt_entry_t pte)
{
	*pte = 0;
}

static inline bool _svX_page_table_empty(page_table_t page_table,
					 unsigned long len)
{
	unsigned long *page_table_end = page_table + len;

	for (; page_table_end > page_table; ++page_table)
		if (RISCV_PTE_FLAG (V) & *page_table)
			return false;
	return true;
}

static bool svX_page_table_empty(page_table_t page_table)
{
	return _svX_page_table_empty(page_table, 1 << PAGE_LEVEL_BITS);
}

#define	RISCV_SVX_PAGING_LEVEL(LEVEL)				\
	{							\
		.page_size = 1UL << UNTRANSLATED_BITS(LEVEL),	\
		.get_entry = svX_vpn ## LEVEL ## _get_entry,	\
		.entry_valid = svX_entry_valid,			\
		.set_terminal = svX_vpnX_set_terminal,		\
		.get_phys = svX_vpn## LEVEL ##_get_phys,	\
		.get_flags = svX_get_flags,			\
		.set_next_pt = svX_vpnX_set_next_pt,		\
		.get_next_pt = svX_vpnX_get_next_pt,		\
		.clear_entry = svX_clear_entry,			\
		.page_table_empty = svX_page_table_empty,	\
	}

/* sequence is from root to leaves */
const struct paging riscv_Sv39[] = {
	RISCV_SVX_PAGING_LEVEL(2),
	RISCV_SVX_PAGING_LEVEL(1),
	RISCV_SVX_PAGING_LEVEL(0),
};

const struct paging riscv_Sv48[] = {
	RISCV_SVX_PAGING_LEVEL(3),
	RISCV_SVX_PAGING_LEVEL(2),
	RISCV_SVX_PAGING_LEVEL(1),
	RISCV_SVX_PAGING_LEVEL(0),
};
