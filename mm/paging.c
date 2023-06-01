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

#define dbg_fmt(x) "page: " x

#include <grinch/errno.h>
#include <grinch/mm.h>
#include <grinch/cpu.h>
#include <grinch/paging.h>
#include <grinch/mmu.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/symbols.h>

/* For later usages */
#define PAGING_COHERENT		0x1
#define PAGING_HUGE		0x2

#define MAX_PAGE_TABLE_LEVELS	4

unsigned long satp_mode;

static const struct paging *root_paging;

struct paging_structures {
	const struct paging *root_paging;
	page_table_t root_table;
};

static inline void paging_flush_page_tlbs(paddr_t page_addr)
{
	asm volatile("sfence.vma /* rd, */ zero, %[addr]"
		     : : [addr] "r" (page_addr));
}

static int paging_create(const struct paging_structures *pg_structs,
		  unsigned long phys, unsigned long size, unsigned long virt,
		  unsigned long access_flags, unsigned long paging_flags);

static int split_hugepage(const struct paging *paging,
			  pt_entry_t pte, unsigned long virt,
			  unsigned long paging_flags)
{
	unsigned long phys = paging->get_phys(pte, virt);
	struct paging_structures sub_structs;
	unsigned long page_mask, flags;

	if (phys == INVALID_PHYS_ADDR)
		return 0;

	page_mask = ~((unsigned long)paging->page_size - 1);
	phys &= page_mask;
	virt &= page_mask;

	flags = paging->get_flags(pte);

	sub_structs.root_paging = paging + 1;
	sub_structs.root_table = page_zalloc(1, PAGE_SIZE, PAF_INT);
	if (!sub_structs.root_table)
		return -ENOMEM;
	paging->set_next_pt(pte, virt_to_phys(sub_structs.root_table));

	return paging_create(&sub_structs, phys, paging->page_size, virt,
			     flags, paging_flags);
}

static int paging_destroy(const struct paging_structures *pg_structs,
		   unsigned long virt, unsigned long size,
		   unsigned long paging_flags)
{
	size = page_up(size);
	virt &= PAGE_MASK;

	while (size > 0) {
		const struct paging *paging = pg_structs->root_paging;
		page_table_t pt[MAX_PAGE_TABLE_LEVELS];
		unsigned long page_size;
		pt_entry_t pte;
		int n = 0;
		int err;

		/* walk down the page table, saving intermediate tables */
		pt[0] = pg_structs->root_table;
		while (1) {
			pte = paging->get_entry(pt[n], virt);
			if (!paging->entry_valid(pte, PAGE_PRESENT_FLAGS))
				break;
			if (paging->get_phys(pte, virt) != INVALID_PHYS_ADDR) {
				unsigned long page_start;

				/*
				 * If the region to be unmapped doesn't fully
				 * cover the hugepage, the hugepage will need to
				 * be split.
				 */
				page_size = paging->page_size ?
					paging->page_size : PAGE_SIZE;
				page_start = virt & ~(page_size-1);

				/*
				 * It's possible that virt + size overflows to
				 * exactly 0 (e.g. a 512MB region starting at
				 * 0xe0000000 with 32-bit addresses) during
				 * normal execution. Any overflow beyond that is
				 * a programming error.
				 *
				 * To handle this case, subtract 1 from the size
				 * when comparing both sides. Note that size and
				 * page_size are always > 0, so there's no risk
				 * of underflow.
				 */
				if (virt <= page_start &&
				    virt + (size - 1) >=
				    page_start + (page_size - 1))
					break;

				err = split_hugepage(paging, pte, virt,
						     paging_flags);
				if (err)
					return err;
			}
			pt[++n] = phys_to_virt(paging->get_next_pt(pte));
			paging++;
		}
		/* advance by page size of current level paging */
		page_size = paging->page_size ? paging->page_size : PAGE_SIZE;

		/* walk up again, clearing entries, releasing empty tables */
		while (1) {
			paging->clear_entry(pte);
			if (n == 0 || !paging->page_table_empty(pt[n]))
				break;
			err = page_free(pt[n], 1, PAF_INT);
			if (err)
				return err;

			paging--;
			pte = paging->get_entry(pt[--n], virt);
		}

		paging_flush_page_tlbs(virt);

		if (page_size > size)
			break;
		virt += page_size;
		size -= page_size;
	}
	return 0;
}

static int paging_create(const struct paging_structures *pg_structs,
		  unsigned long phys, unsigned long size, unsigned long virt,
		  unsigned long access_flags, unsigned long paging_flags)
{
	phys &= PAGE_MASK;
	virt &= PAGE_MASK;
	size = page_up(size);

	while (size > 0) {
		const struct paging *paging = pg_structs->root_paging;
		page_table_t pt = pg_structs->root_table;
		struct paging_structures sub_structs;
		pt_entry_t pte;
		int err;

		while (1) {
			pte = paging->get_entry(pt, virt);
			if (paging->page_size > 0 &&
			    paging->page_size <= size &&
			    ((phys | virt) & (paging->page_size - 1)) == 0 &&
			    (paging_flags & PAGING_HUGE ||
			     paging->page_size == PAGE_SIZE)) {
				/*
				 * We might be overwriting a more fine-grained
				 * mapping, so release it first. This cannot
				 * fail as we are working along hugepage
				 * boundaries.
				 */
				if (paging->page_size > PAGE_SIZE) {
					sub_structs.root_paging = paging;
					sub_structs.root_table = pt;
					paging_destroy(&sub_structs, virt,
						       paging->page_size,
						       paging_flags);
				}
				paging->set_terminal(pte, phys, access_flags);
				break;
			}
			if (paging->entry_valid(pte, PAGE_PRESENT_FLAGS)) {
				err = split_hugepage(paging, pte, virt,
						     paging_flags);
				if (err)
					return err;
				pt = phys_to_virt(paging->get_next_pt(pte));
			} else {
				pt = page_zalloc(1, PAGE_SIZE, PAF_INT);
				if (!pt)
					return -ENOMEM;
				paging->set_next_pt(pte,
						    virt_to_phys(pt));
			}
			paging++;
		}

		paging_flush_page_tlbs(virt);

		phys += paging->page_size;
		virt += paging->page_size;
		size -= paging->page_size;
	}
	return 0;
}

int unmap_range(page_table_t pt, const void *vaddr, size_t size)
{
	struct paging_structures pg = {
		.root_paging = root_paging,
		.root_table = pt,
	};

	pr("Unmapping VA: 0x%llx PA: 0x%llx (SZ: 0x%lx)\n",
	   (u64)vaddr, virt_to_phys(vaddr), size);

	return paging_destroy(&pg, (unsigned long)vaddr, size, 0);
}

int map_range(page_table_t pt, const void *vaddr, paddr_t paddr, size_t size, u16 flags)
{
	struct paging_structures pg = {
		.root_paging = root_paging,
		.root_table = pt,
	};

	pr("Create mapping VA: 0x%llx PA: 0x%llx (%c%c%c%c SZ: 0x%lx)\n",
	   (u64)vaddr, (u64)paddr,
	   flags & RISCV_PTE_FLAG(R) ? 'R' : '-',
	   flags & RISCV_PTE_FLAG(W) ? 'W' : '-',
	   flags & RISCV_PTE_FLAG(X) ? 'X' : '-',
	   flags & RISCV_PTE_FLAG(U) ? 'U' : '-',
	   size);

	return paging_create(&pg, paddr, size, (unsigned long)vaddr, flags, 0);
}

static int map_osmem(page_table_t root, void *vaddr, size_t size, u16 flags)
{
	return map_range(root, vaddr, virt_to_phys(vaddr), size, flags);
}

int paging_cpu_init(unsigned long hart_id)
{
	int err;
	page_table_t root;

	root = per_cpu(hart_id)->root_table_page;

	/* Map the stack */
	err = map_range(root, (void*)PERCPU_BASE,
			virt_to_phys(per_cpu(hart_id)),
			sizeof(struct per_cpu),
			PAGE_FLAGS_MEM_RW);

	return err;
}

int paging_init(void)
{
	int err;
	page_table_t root;

	/* SV39 should suffice for everything */
	root_paging = riscv_Sv39;
	satp_mode = SATP_MODE_39;

	root = this_root_table_page();

	/* root tables are not in bss section, so zero them */
	memset(root, 0, sizeof(this_per_cpu()->root_table_page));

	err = map_osmem(root, _load_addr,
			page_up(__text_end - __load_addr),
			PAGE_FLAGS_MEM_RX);
	if (err)
		goto out;

	err = map_osmem(root, __rw_data_start,
			page_up(__rw_data_end - __rw_data_start),
			PAGE_FLAGS_MEM_RW);
	if (err)
		goto out;

	err = map_osmem(root, __rodata_start,
			page_up(__rodata_end - __rodata_start),
			PAGE_FLAGS_MEM_RO);
	if (err)
		goto out;

	/* Map the page pool */
	err = map_osmem(root, __internal_page_pool_start,
			internal_page_pool_pages() * PAGE_SIZE,
			PAGE_FLAGS_MEM_RW);
	if (err)
		goto out;

	err = paging_cpu_init(this_cpu_id());
	if (err)
		goto out;

	ps("Activating MMU...\n");
	enable_mmu_satp(satp_mode, virt_to_phys(root));

	return 0;

out:
	pr("Mapping error: %d\n", err);
	return err;
}
