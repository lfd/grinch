/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2020
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Konrad Schwarz <konrad.schwarz@siemens.com>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/gfp.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>

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

unsigned long hgatp_mode;
static unsigned long satp_mode;

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
static const struct paging riscv_Sv39[] = {
	RISCV_SVX_PAGING_LEVEL(2),
	RISCV_SVX_PAGING_LEVEL(1),
	RISCV_SVX_PAGING_LEVEL(0),
};

static const struct paging riscv_Sv48[] = {
	RISCV_SVX_PAGING_LEVEL(3),
	RISCV_SVX_PAGING_LEVEL(2),
	RISCV_SVX_PAGING_LEVEL(1),
	RISCV_SVX_PAGING_LEVEL(0),
};


/* For the rest (non-root tbls), reuse svX routines */

/*** sv39x ***/
/* 4K*2 for level 2 */
DEF_GET_ENTRY(39x4, 2, true)
#define sv39x4_vpn0_get_entry	svX_vpn0_get_entry
#define sv39x4_vpn0_get_phys	svX_vpn0_get_phys

#define sv39x4_vpn1_get_entry	svX_vpn1_get_entry
#define sv39x4_vpn1_get_phys	svX_vpn1_get_phys

#define sv39x4_vpn2_get_phys	svX_vpn2_get_phys

/*** sv48x ***/
/* 4K*2 for level 3 */
DEF_GET_ENTRY(48x4, 3, true)
#define sv48x4_vpn0_get_entry	svX_vpn0_get_entry
#define sv48x4_vpn0_get_phys	svX_vpn0_get_phys

#define sv48x4_vpn1_get_entry	svX_vpn1_get_entry
#define sv48x4_vpn1_get_phys	svX_vpn1_get_phys

#define sv48x4_vpn2_get_entry	svX_vpn2_get_entry
#define sv48x4_vpn2_get_phys	svX_vpn2_get_phys

#define sv48x4_vpn3_get_phys	svX_vpn3_get_phys


#define RISCV_SVXx4_PAGING_LEVEL(WIDTH, LEVEL, ROOT)            \
	{                                                       \
		1UL << UNTRANSLATED_BITS(LEVEL),                \
		sv ## WIDTH ## x4_vpn ## LEVEL ## _get_entry,   \
		svX_entry_valid,                                \
		svXx4_vpnX_set_terminal,                        \
		sv ## WIDTH ## x4_vpn ## LEVEL ## _get_phys,    \
		svX_get_flags,                                  \
		svXx4_vpnX_set_next_pt,                         \
		svX_vpnX_get_next_pt,                           \
		svX_clear_entry,                                \
		(ROOT)? svXx4_root_page_table_empty:            \
				svX_page_table_empty,           \
	}

DEF_SET_TERMINAL(Xx4, RISCV_PTE_FLAG(U))

DEF_SET_NEXT(Xx4, 0)

static bool svXx4_root_page_table_empty(page_table_t page_table)
{
	return _svX_page_table_empty(page_table, 2 << (2 + PAGE_LEVEL_BITS));
}

const struct paging riscv_Sv39x4[] = {
	RISCV_SVXx4_PAGING_LEVEL(39, 2, true),
	RISCV_SVXx4_PAGING_LEVEL(39, 1, false),
	RISCV_SVXx4_PAGING_LEVEL(39, 0, false),
};

const struct paging riscv_Sv48x4[] = {
	RISCV_SVXx4_PAGING_LEVEL(48, 3, true),
	RISCV_SVXx4_PAGING_LEVEL(48, 2, false),
	RISCV_SVXx4_PAGING_LEVEL(48, 1, false),
	RISCV_SVXx4_PAGING_LEVEL(48, 0, false),
};

void __init arch_paging_init(void)
{
	/* SV39 should suffice for everything */
	if (1) {
		root_paging = riscv_Sv39;
		satp_mode = SATP_MODE_39;

		vm_paging = riscv_Sv39x4;
		hgatp_mode = SATP_MODE_39;
	} else {
		root_paging = riscv_Sv48;
		satp_mode = SATP_MODE_48;

		vm_paging = riscv_Sv48x4;
		hgatp_mode = SATP_MODE_48;
	}
}

void arch_paging_enable(unsigned long this_cpu, page_table_t pt)
{
	paddr_t tmp;
	enable_mmu_satp(satp_mode, v2p(pt));

	/* Get the stack pointer under control */
	tmp = PERCPU_BASE - (paddr_t)per_cpu(this_cpu);
	asm volatile("add sp, sp, %0" : : "r"(tmp));
}
