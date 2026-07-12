/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/const.h>
#include <grinch/types.h>
#include <asm-generic/grinch_layout.h>
#include <asm/sysregs.h>
#include <asm/cpu.h>

#include <grinch/init.h>
#include <grinch/loader.h>
#include <grinch/symbols.h>

void __noreturn grinch_start(u64 p_grinch_dst, u64 p_fdt);
void __noreturn __init loader(paddr_t fdt, paddr_t load_addr);

enum map_type { MAP_CACHED, MAP_UNCACHED };

static void panic(void)
{
	while(1)
		cpu_relax();
}

static u64 * __init walk_to_l2(void **next, u64 *page_directory, paddr_t vaddr)
{
	u64 *l1, *l2;
	unsigned index;

	index = L0_IDX(vaddr);
	if (!(page_directory[index] & PTE_TABLE_FLAGS)) {
		l1 = loader_page_zalloc(next);
		/* ensure the page table walker will see the zeroes */
		synchronization_barrier();
		page_directory[index] = (unsigned long)l1 | PTE_TABLE_FLAGS;
	} else {
		l1 = (u64*)(unsigned long)(page_directory[index] & ~PTE_TABLE_FLAGS);
	}

	index = L1_IDX(vaddr);
	if (!(l1[index] & PTE_TABLE_FLAGS)) {
		l2 = loader_page_zalloc(next);
		synchronization_barrier();
		l1[index] = (unsigned long)l2 | PTE_TABLE_FLAGS;
	} else {
		l2 = (u64*)(unsigned long)(l1[index] & ~PTE_TABLE_FLAGS);
	}

	return l2;
}

static void __init map_huge_page(void **next, u64 *page_directory, void *vaddr,
				 u64 paddr, enum map_type map_type)
{
	paddr_t _vaddr = (paddr_t)vaddr;
	u64 *l2, entry;

	if (_vaddr & MEGA_PAGE_OFFS_MASK)
		panic();
	if (paddr & MEGA_PAGE_OFFS_MASK)
		panic();

	l2 = walk_to_l2(next, page_directory, _vaddr);

	entry = paddr | PTE_ACCESS_FLAG | PTE_INNER_SHAREABLE |
		S1_PTE_ACCESS_RW | PTE_BLOCK_FLAGS;
	if (map_type == MAP_CACHED)
		entry |= S1_PTE_FLAG_NORMAL;
	else
		entry |= S1_PTE_FLAG_DEVICE;

	l2[L2_IDX(_vaddr)] = entry;

	/*
	 * As long as we only add entries and do not modify entries, a
	 * synchronization barrier is enough to propagate changes. Otherwise
	 * we need to flush the TLB.
	 */
	synchronization_barrier();
}

static void __init map_page(void **next, u64 *page_directory, void *vaddr,
			    u64 paddr, enum map_type map_type)
{
	paddr_t _vaddr = (paddr_t)vaddr;
	u64 *l2, *l3, entry;
	unsigned index;

	if (_vaddr & PAGE_OFFS_MASK)
		panic();
	if (paddr & PAGE_OFFS_MASK)
		panic();

	l2 = walk_to_l2(next, page_directory, _vaddr);

	index = L2_IDX(_vaddr);
	if (!(l2[index] & PTE_TABLE_FLAGS)) {
		l3 = loader_page_zalloc(next);
		synchronization_barrier();
		l2[index] = (unsigned long)l3 | PTE_TABLE_FLAGS;
	} else {
		l3 = (u64*)(unsigned long)(l2[index] & ~PTE_TABLE_FLAGS);
	}

	entry = paddr | PTE_ACCESS_FLAG | PTE_INNER_SHAREABLE |
		S1_PTE_ACCESS_RW | PTE_TABLE_FLAGS;
	if (map_type == MAP_CACHED)
		entry |= S1_PTE_FLAG_NORMAL;
	else
		entry |= S1_PTE_FLAG_DEVICE;

	l3[L3_IDX(_vaddr)] = entry;

	synchronization_barrier();
}

void __noreturn __init loader(paddr_t fdt, paddr_t load_addr)
{
	u64 *page_directory_0, *page_directory_1;
	unsigned long sctlr;
	unsigned int d;
	paddr_t tmp;
	void *next;
	u64 offset;

	next = (void *)(uintptr_t)(load_addr + GRINCH_SIZE);
	page_directory_0 = loader_page_zalloc(&next); /* TTBR0: identity map */
	page_directory_1 = loader_page_zalloc(&next); /* TTBR1: virtual map  */

	for (d = 0; d + MEGA_PAGE_SIZE <= GRINCH_SIZE; d += MEGA_PAGE_SIZE) {
		tmp = load_addr + d;
		map_huge_page(&next, page_directory_0, (void *)tmp, tmp, MAP_CACHED);
		map_huge_page(&next, page_directory_1, (void *)GRINCH_BASE + d, tmp, MAP_CACHED);
	}
	for (; d < GRINCH_SIZE; d += PAGE_SIZE) {
		tmp = load_addr + d;
		map_page(&next, page_directory_0, (void *)tmp, tmp, MAP_CACHED);
		map_page(&next, page_directory_1, (void *)GRINCH_BASE + d, tmp, MAP_CACHED);
	}

	arm_write_sysreg(MAIR, MAIR0_DEFAULT);
	arm_write_sysreg(TCR, TCR_SETTINGS);
	arm_write_sysreg(TTBR0, page_directory_0);
	arm_write_sysreg(TTBR1, page_directory_1);
	instruction_barrier();

	arm_read_sysreg(SCTLR, sctlr);
	sctlr |= SCTLR_MMU_CACHES;
	arm_write_sysreg(SCTLR, sctlr);
	instruction_barrier();

	offset = (paddr_t)__start - load_addr;
	asm volatile(
		"add	sp, sp, %[offset]\n"
		"ldr	x0, =virt_start\n"
		"br	x0\n"
		"virt_start:\n"
	: : [offset] "r"(offset) : "x0");

	grinch_start(load_addr, fdt);
}
