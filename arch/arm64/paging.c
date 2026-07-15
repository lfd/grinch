/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/cpu.h>
#include <asm/sysregs.h>

#include <grinch/asid.h>
#include <grinch/gfp.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>

#define BLOCK_2M_VADDR_MASK	GENMASK_ULL(20, 0)
#define BLOCK_1G_VADDR_MASK	GENMASK_ULL(29, 0)
#define BLOCK_512G_VADDR_MASK	GENMASK_ULL(38, 0)

#define PTE_TABLE_ADDR_MASK	GENMASK_ULL(47, 12)
#define PTE_PAGE_ADDR_MASK	GENMASK_ULL(47, 12)
#define PTE_L0_BLOCK_ADDR_MASK	GENMASK_ULL(47, 39)
#define PTE_L1_BLOCK_ADDR_MASK	GENMASK_ULL(47, 30)
#define PTE_L2_BLOCK_ADDR_MASK	GENMASK_ULL(47, 21)

/* Probed from ID_AA64MMFR0_EL1 in arch_paging_enable(). */
unsigned long asid_mask;

static bool arm_entry_valid(pt_entry_t entry, unsigned long flags)
{
	return *entry & 1;
}

static unsigned long arm_get_entry_flags(pt_entry_t entry)
{
	/* Upper flags (contiguous hint and XN are currently ignored */
	return *entry & 0xfff;
}

static void arm_clear_entry(pt_entry_t entry)
{
	*entry = 0;
}

static bool arm_page_table_empty(page_table_t page_table)
{
	unsigned long n;
	pt_entry_t pte;

	for (n = 0, pte = page_table; n < PAGE_SIZE / sizeof(u64); n++, pte++)
		if (arm_entry_valid(pte, PTE_FLAG_VALID))
			return false;
	return true;
}

static pt_entry_t arm_get_l0_entry(page_table_t page_table, unsigned long virt)
{
	return &page_table[L0_IDX(virt)];
}

static unsigned long arm_get_l0_phys(pt_entry_t pte, unsigned long virt)
{
	if ((*pte & PTE_TABLE_FLAGS) == PTE_TABLE_FLAGS)
		return INVALID_PHYS_ADDR;
	return (*pte & PTE_L0_BLOCK_ADDR_MASK) | (virt & BLOCK_512G_VADDR_MASK);
}

static pt_entry_t arm_get_l1_entry(page_table_t page_table, unsigned long virt)
{
	return &page_table[L1_IDX(virt)];
}

static void arm_set_l1_block(pt_entry_t pte, unsigned long phys, unsigned long flags)
{
	*pte = ((u64)phys & PTE_L1_BLOCK_ADDR_MASK) | flags;
}

static unsigned long arm_get_l1_phys(pt_entry_t pte, unsigned long virt)
{
	if ((*pte & PTE_TABLE_FLAGS) == PTE_TABLE_FLAGS)
		return INVALID_PHYS_ADDR;
	return (*pte & PTE_L1_BLOCK_ADDR_MASK) | (virt & BLOCK_1G_VADDR_MASK);
}

static pt_entry_t arm_get_l2_entry(page_table_t page_table, unsigned long virt)
{
	return &page_table[L2_IDX(virt)];
}

static pt_entry_t arm_get_l3_entry(page_table_t page_table, unsigned long virt)
{
	return &page_table[L3_IDX(virt)];
}

static void arm_set_l2_block(pt_entry_t pte, unsigned long phys, unsigned long flags)
{
	*pte = ((u64)phys & PTE_L2_BLOCK_ADDR_MASK) | flags;
}

static void arm_set_l3_page(pt_entry_t pte, unsigned long phys, unsigned long flags)
{
	*pte = ((u64)phys & PTE_PAGE_ADDR_MASK) | flags | PTE_FLAG_TERMINAL;
}

static void arm_set_l12_table(pt_entry_t pte, unsigned long next_pt)
{
	*pte = ((u64)next_pt & PTE_TABLE_ADDR_MASK) | PTE_TABLE_FLAGS;
}

static unsigned long arm_get_l12_table(pt_entry_t pte)
{
	return *pte & PTE_TABLE_ADDR_MASK;
}

static unsigned long arm_get_l2_phys(pt_entry_t pte, unsigned long virt)
{
	if ((*pte & PTE_TABLE_FLAGS) == PTE_TABLE_FLAGS)
		return INVALID_PHYS_ADDR;
	return (*pte & PTE_L2_BLOCK_ADDR_MASK) | (virt & BLOCK_2M_VADDR_MASK);
}

static unsigned long arm_get_l3_phys(pt_entry_t pte, unsigned long virt)
{
	if (!(*pte & PTE_FLAG_TERMINAL))
		return INVALID_PHYS_ADDR;
	return (*pte & PTE_PAGE_ADDR_MASK) | (virt & PAGE_OFFS_MASK);
}

#define ARM_PAGING_COMMON				\
		.entry_valid = arm_entry_valid,		\
		.get_flags = arm_get_entry_flags,	\
		.clear_entry = arm_clear_entry,		\
		.page_table_empty = arm_page_table_empty,

static const struct paging arm_paging[] = {
	{
		ARM_PAGING_COMMON
		/* No block entries for level 0, so no need to set page_size */
		.get_entry = arm_get_l0_entry,
		.get_phys = arm_get_l0_phys,

		.set_next_pt = arm_set_l12_table,
		.get_next_pt = arm_get_l12_table,
	},
	{
		ARM_PAGING_COMMON
		/* Block entry: 1GB */
		.page_size = 1024 * 1024 * 1024,
		.get_entry = arm_get_l1_entry,
		.set_terminal = arm_set_l1_block,
		.get_phys = arm_get_l1_phys,

		.set_next_pt = arm_set_l12_table,
		.get_next_pt = arm_get_l12_table,
	},
	{
		ARM_PAGING_COMMON
		/* Block entry: 2MB */
		.page_size = 2 * 1024 * 1024,
		.get_entry = arm_get_l2_entry,
		.set_terminal = arm_set_l2_block,
		.get_phys = arm_get_l2_phys,

		.set_next_pt = arm_set_l12_table,
		.get_next_pt = arm_get_l12_table,
	},
	{
		ARM_PAGING_COMMON
		/* Page entry: 4kB */
		.page_size = 4 * 1024,
		.get_entry = arm_get_l3_entry,
		.set_terminal = arm_set_l3_page,
		.get_phys = arm_get_l3_phys,
	}
};

void arch_paging_init(void)
{
	root_paging = arm_paging;
}

unsigned long arch_nr_asids(void)
{
	return asid_mask + 1;
}

void arch_paging_enable(unsigned long this_cpu, page_table_t pt)
{
	static bool asid_probed;
	unsigned long mmfr0, tcr;

	instruction_barrier();
	arm_write_sysreg(TTBR1, v2p(pt));
	instruction_barrier();
	flush_tlb_all();

	/* Probe how many ASID bits the implementation provides - once. */
	if (!asid_probed) {
		asid_probed = true;

		arm_read_sysreg(ID_AA64MMFR0_EL1, mmfr0);
		if (((mmfr0 >> ID_AA64MMFR0_ASID_SHIFT) &
		     ID_AA64MMFR0_ASID_MASK) == ID_AA64MMFR0_ASID_16)
			asid_mask = 0xffff;
		else
			asid_mask = 0xff;
	}

	/*
	 * TCR is per-CPU state: with 16-bit tags in use, every CPU must
	 * compare all sixteen bits (TCR.AS), or address spaces whose
	 * ASIDs differ only in the upper byte alias in its TLB.
	 */
	if (asid_mask > 0xff) {
		arm_read_sysreg(TCR, tcr);
		arm_write_sysreg(TCR, tcr | TCR_EL1_AS);
		instruction_barrier();
	}

	/*
	 * Set exception stack pointer: We share the stack between exceptions
	 * and regular kernel code.
	 */
	asm volatile(
		"mov x0, sp\n"
		"msr spsel, %0\n"
		"mov sp, x0\n"
		: : "r"(1) : "x0");
}
