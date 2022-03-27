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

#include <grinch/compiler_attributes.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/grinch_layout.h>
#include <grinch/symbols.h>

ptrdiff_t setup_trampoline(unsigned long hart_id, paddr_t load_addr, void *vbase);

static __always_inline void map_2M(u64 *l0, u64 *l1, void *vaddr,
				   paddr_t paddr)
{
	register u64 *l0_entry = &l0[vaddr2vpn(vaddr, 2)];
	register u64 *l1_entry;

	if (*l0_entry & RISCV_PTE_FLAG(V)) {
		l1 = (u64*)pte2table(*l0_entry);
	} else {
		*l0_entry = paddr2pte((paddr_t)l1) | PAGE_PRESENT_FLAGS;
	}

	l1_entry = &l1[vaddr2vpn(vaddr, 1)];
	*l1_entry = paddr2pte(paddr) | PAGE_FLAGS_DEFAULT;
}

static __always_inline void map_4K(u64 *l0, u64 *l1, u64 *l2, void *vaddr,
				   paddr_t paddr)
{
	register u64 *l0_entry = &l0[vaddr2vpn(vaddr, 2)];
	register u64 *l1_entry, *l2_entry;

	if (*l0_entry & RISCV_PTE_FLAG(V)) {
		l1 = (u64*)pte2table(*l0_entry);
	} else {
		*l0_entry = paddr2pte((paddr_t)l1) | PAGE_PRESENT_FLAGS;
	}

	l1_entry = &l1[vaddr2vpn(vaddr, 1)];
	if (*l1_entry & RISCV_PTE_FLAG(V)) {
		l2 = (u64*)pte2table(*l1_entry);
	} else {
		*l1_entry = paddr2pte((paddr_t)l2) | PAGE_PRESENT_FLAGS;
	}

	l2_entry = &l2[vaddr2vpn(vaddr, 0)];
	*l2_entry = paddr2pte(paddr) | PAGE_FLAGS_DEFAULT;
}

ptrdiff_t setup_trampoline(unsigned long hart_id, paddr_t load_addr, void *vbase)
{
	/*
	 * Locate the trampoline some pages inside the page pool. In this way,
	 * we can use alloc while setting up the final paging. Later, those
	 * pages can be overwritten, as they are dropped.
	 */

	/*
	 * allocate three page tables. mm_init() will take care that those
	 * pages aren't used.
	 */
	register u64 (*table)[PTES_PER_PT] =
		(void*)(__internal_page_pool_start - __load_addr + load_addr);
	register unsigned long stack_phys;
	register unsigned int d;
	register ptrdiff_t v2p;

	/* zero bootstrapping page tables */
	memset(table[0], 0, PIE_PAGES * sizeof(table[0]));

	for (d = 0; d < mega_page_up(__internal_page_pool_end - __load_addr);
	     d+= MEGA_PAGE_SIZE) {
		/* ID map loaded location */
		map_2M(table[0], table[1], (void*)load_addr + d, load_addr + d);

		/* Map final destination */
		map_2M(table[0], table[2], vbase + d, load_addr + d);
	}

	stack_phys = __internal_page_pool_end -
		(hart_id + 1) * sizeof(struct per_cpu) -
		__load_addr + load_addr;

	for (d = 0; d < sizeof(struct per_cpu); d+= PAGE_SIZE)
		map_4K(table[0], table[3], table[4], (void*)PERCPU_BASE + d,
		       stack_phys + d);

	enable_mmu_satp(SATP_MODE_39, (paddr_t)table);

	this_per_cpu()->hartid = hart_id;

	v2p = (paddr_t)vbase - load_addr;
	return v2p;
}
