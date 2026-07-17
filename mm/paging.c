/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "page: " x

#include <grinch/cpu.h>
#include <grinch/gfp.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/symbols.h>
#include <grinch/task.h>

/* For later usages */
#define PAGING_COHERENT		0x1
#define PAGING_HUGE		0x2

#define MAX_PAGE_TABLE_LEVELS	4

const struct paging *root_paging, *vm_paging;

/*
 * Which translations a page table modification can affect, i.e. what
 * has to be flushed from the TLB.
 */
enum tlb_scope {
	TLB_SCOPE_NONE,		/* no hardware walker reaches the tables */
	TLB_SCOPE_LOCAL,	/* live on this CPU */
	TLB_SCOPE_GUEST,	/* guest (stage-2) translations */
};

struct paging_structures {
	const struct paging *root_paging;
	page_table_t root_table;
	enum tlb_scope scope;
};

/*
 * A process' page table is the translation root while its process is
 * current on this CPU. The kernel root stays live under any root:
 * activation installs its entries, and they share their lower-level
 * tables across all roots.
 *
 * Translations of a process that ran earlier may linger on other CPUs
 * until they activate their next task. That is tolerable: user
 * mappings are only architecturally accessed while their process runs,
 * and activation performs a full local flush.
 */
static enum tlb_scope tlb_scope_of(page_table_t pt)
{
	struct task *task;

	if (pt == this_root_table_page())
		return TLB_SCOPE_LOCAL;

	task = current_task();
	if (task && task->type == GRINCH_PROCESS &&
	    task->process.mm.page_table == pt)
		return TLB_SCOPE_LOCAL;

	return TLB_SCOPE_NONE;
}

static int paging_create(const struct paging_structures *pg_structs,
		  unsigned long phys, unsigned long size, unsigned long virt,
		  unsigned long access_flags, unsigned long paging_flags);

/*
 * VA range covered by one entry of the given level. Levels without
 * terminal entries (page_size == 0) derive their reach from the levels
 * below.
 */
static unsigned long paging_slot_size(const struct paging *paging)
{
	unsigned long size = 1;

	while (!paging->page_size) {
		size *= PTES_PER_PT;
		paging++;
	}

	return size * paging->page_size;
}

static int split_hugepage(const struct paging *paging,
			  pt_entry_t pte, unsigned long virt,
			  unsigned long paging_flags, enum tlb_scope scope)
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
	sub_structs.root_table = zalloc_pages(1);
	sub_structs.scope = scope;
	if (!sub_structs.root_table)
		return -ENOMEM;
	paging->set_next_pt(pte, v2p(sub_structs.root_table));

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
		page_table_t empty[MAX_PAGE_TABLE_LEVELS];
		unsigned long page_size, advance;
		int n_empty = 0;
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
						     paging_flags,
						     pg_structs->scope);
				if (err)
					return err;
			}
			pt[++n] = p2v(paging->get_next_pt(pte));
			paging++;
		}
		/*
		 * Advance to the end of the slot the walk stopped in. If
		 * the walk stopped early - an invalid entry, or a terminal
		 * entry above the leaf level - everything up to the next
		 * slot boundary of that level is covered in one step.
		 */
		page_size = paging_slot_size(paging);
		advance = page_size - (virt & (page_size - 1));

		/* walk up again, clearing entries, collecting empty tables */
		while (1) {
			paging->clear_entry(pte);
			if (n == 0 || !paging->page_table_empty(pt[n]))
				break;
			empty[n_empty++] = pt[n];

			paging--;
			pte = paging->get_entry(pt[--n], virt);
		}

		/*
		 * If page tables were emptied, a per-address flush is not
		 * enough: walk caches may still reference the dead tables
		 * for other addresses within their reach.
		 */
		switch (pg_structs->scope) {
		case TLB_SCOPE_NONE:
			break;
		case TLB_SCOPE_LOCAL:
			if (n_empty)
				local_flush_tlb_all();
			else
				local_flush_tlb_page(virt);
			break;
		case TLB_SCOPE_GUEST:
			local_flush_tlb_guest_all();
			break;
		}

		/*
		 * Release emptied page tables only after the flush; until
		 * then, hardware walkers may still traverse them.
		 */
		while (n_empty > 0) {
			err = free_pages(empty[--n_empty], 1);
			if (err)
				return err;
		}

		if (advance >= size)
			break;
		virt += advance;
		size -= advance;
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
					sub_structs.scope = pg_structs->scope;
					paging_destroy(&sub_structs, virt,
						       paging->page_size,
						       paging_flags);
				}
				paging->set_terminal(pte, phys, access_flags);
				break;
			}
			if (paging->entry_valid(pte, PAGE_PRESENT_FLAGS)) {
				err = split_hugepage(paging, pte, virt,
						     paging_flags,
						     pg_structs->scope);
				if (err)
					return err;
				pt = p2v(paging->get_next_pt(pte));
			} else {
				pt = zalloc_pages(1);
				if (!pt)
					return -ENOMEM;
				paging->set_next_pt(pte, v2p(pt));
			}
			paging++;
		}

		switch (pg_structs->scope) {
		case TLB_SCOPE_NONE:
			break;
		case TLB_SCOPE_LOCAL:
			local_flush_tlb_page(virt);
			break;
		case TLB_SCOPE_GUEST:
			local_flush_tlb_guest_all();
			break;
		}

		phys += paging->page_size;
		virt += paging->page_size;
		size -= paging->page_size;
	}
	return 0;
}

static int _unmap_range(struct paging_structures *pg, const void *vaddr, size_t size)
{
	pr_dbg("Unmapping VA: 0x%lx (SZ: 0x%lx)\n", (uintptr_t)vaddr, size);

	return paging_destroy(pg, (unsigned long)vaddr, size, 0);
}

static int _map_range(struct paging_structures *pg, const void *vaddr,
		      paddr_t paddr, size_t size, mem_flags_t grinch_flags)
{
	unsigned long flags;

	pr_dbg("Create mapping VA: 0x%lx PA: 0x%lx (%c%c%c%c%c SZ: 0x%lx)\n",
	       (uintptr_t)vaddr, paddr,
	       grinch_flags & GRINCH_MEM_R ? 'R' : '-',
	       grinch_flags & GRINCH_MEM_W ? 'W' : '-',
	       grinch_flags & GRINCH_MEM_X ? 'X' : '-',
	       grinch_flags & GRINCH_MEM_U ? 'U' : '-',
	       grinch_flags & GRINCH_MEM_DEVICE ? 'D' : '-',
	       size);
	flags = arch_paging_access_flags(grinch_flags);

	return paging_create(pg, paddr, size, (unsigned long)vaddr, flags, PAGING_HUGE);
}

int unmap_range(page_table_t pt, const void *vaddr, size_t size)
{
	struct paging_structures pg = {
		.root_paging = root_paging,
		.root_table = pt,
		.scope = tlb_scope_of(pt),
	};

	return _unmap_range(&pg, vaddr, size);
}

int map_range(page_table_t pt, const void *vaddr, paddr_t paddr, size_t size,
	      mem_flags_t grinch_flags)
{
	struct paging_structures pg = {
		.root_paging = root_paging,
		.root_table = pt,
		.scope = tlb_scope_of(pt),
	};

	return _map_range(&pg, vaddr, paddr, size, grinch_flags);

}

int vm_unmap_range(page_table_t pt, const void *vaddr, size_t size)
{
	struct paging_structures pg = {
		.root_paging = vm_paging,
		.root_table = pt,
		.scope = TLB_SCOPE_GUEST,
	};

	return _unmap_range(&pg, vaddr, size);
}

int vm_map_range(page_table_t pt, const void *vaddr, paddr_t paddr,
		 size_t size, mem_flags_t grinch_flags)
{
	struct paging_structures pg = {
		.root_paging = vm_paging,
		.root_table = pt,
		.scope = TLB_SCOPE_GUEST,
	};

	return _map_range(&pg, vaddr, paddr, size, grinch_flags);
}

static int map_osmem(page_table_t root, void *vaddr, size_t size,
		     mem_flags_t flags)
{
	return map_range(root, vaddr, v2p(vaddr), size, flags);
}

paddr_t paging_get_phys(page_table_t pt, const void *_virt)
{
	const struct paging *paging;
	unsigned long virt;
	pt_entry_t pte;
	paddr_t phys;

	paging = root_paging;
	virt = (unsigned long)_virt;

	while (1) {
		pte = paging->get_entry(pt, virt);
		if (!paging->entry_valid(pte, PAGE_PRESENT_FLAGS))
			return INVALID_PHYS_ADDR;

		phys = paging->get_phys(pte, virt);
		if (phys != INVALID_PHYS_ADDR)
			return phys;

		pt = p2v(paging->get_next_pt(pte));
		paging++;
	}

	return INVALID_PHYS_ADDR;
}

int paging_discard_init(void)
{
	page_table_t root;
	size_t size;
	int err;

	root = this_per_cpu()->root_table_page;
	size = page_up(__init_rw_end - __init_text_start);
	pri("Freeing %lu bytes of init code\n", size);
	err = map_osmem(root, __init_text_start, size, GRINCH_MEM_RW);
	if (err)
		return err;
	flush_tlb_all();

	err = free_pages(__init_text_start, PAGES(size));
	if (err)
		return err;

	return 0;
}

int __init paging_init(unsigned long this_cpu)
{
	int err;
	page_table_t root;

	arch_paging_init();

	pri("=== Grinch memory layout ===\n");
	pri(" Grinch area: 0x%lx -- 0x%lx\n", GRINCH_BASE, GRINCH_END);
	pri("ioremap area: 0x%lx -- 0x%lx\n", IOREMAP_BASE, IOREMAP_END);
	pri("  kheap area: 0x%lx\n", KHEAP_BASE);
	pri(" direct phys: 0x%lx\n", DIR_PHYS_BASE);
	pri("=== Grinch memory layout end ===\n");

	root = per_cpu(this_cpu)->root_table_page;

	err = map_osmem(root, grinch_base(),
			page_up(__text_end - grinch_base()),
			GRINCH_MEM_RX);
	if (err)
		goto out;

	err = map_osmem(root, __rw_data_start,
			page_up(__rw_data_end - __rw_data_start),
			GRINCH_MEM_RW);
	if (err)
		goto out;

	err = map_osmem(root, __rodata_start,
			page_up(__rodata_end - __rodata_start),
			GRINCH_MEM_R);
	if (err)
		goto out;

	err = map_osmem(root, __init_start,
			page_up(__init_ro_end - __init_start),
			GRINCH_MEM_R);
	if (err)
		goto out;

	err = map_osmem(root, __init_rw_start,
			page_up(__init_rw_end - __init_rw_start),
			GRINCH_MEM_RW);
	if (err)
		goto out;

	/* Map the page pool */
	err = map_osmem(root, __internal_page_pool_start,
			internal_page_pool_pages() * PAGE_SIZE,
			GRINCH_MEM_RW);
	if (err)
		goto out;

	arch_paging_enable(this_cpu, root);

	this_per_cpu()->cpuid = this_cpu;

	return 0;

out:
	pri("Mapping error: %pe\n", ERR_PTR(err));
	return err;
}

/*
 * Populate all invalid root-level slots covering [vaddr, vaddr + size)
 * with empty page tables, so that later mappings in that range never
 * have to touch the root level.
 */
int paging_prealloc(page_table_t pt, const void *vaddr, size_t size)
{
	const struct paging *root = root_paging;
	unsigned long virt, slots, slot_size;
	page_table_t sub;
	pt_entry_t pte;

	if (!size)
		return 0;

	/*
	 * virt + slot_size may wrap for ranges in the uppermost slot, so
	 * count slots instead of comparing addresses.
	 */
	slot_size = paging_slot_size(root);
	virt = (unsigned long)vaddr & ~(slot_size - 1);
	slots = ((unsigned long)vaddr + size - 1 - virt) / slot_size + 1;
	for (; slots > 0; slots--, virt += slot_size) {
		pte = root->get_entry(pt, virt);
		if (root->entry_valid(pte, PAGE_PRESENT_FLAGS))
			continue;

		sub = zalloc_pages(1);
		if (!sub)
			return -ENOMEM;
		root->set_next_pt(pte, v2p(sub));
	}

	return 0;
}

/*
 * Warning: In case of errors, this routine does no housekeeping at the moment,
 * and may leak memory!
 */
int paging_duplicate(page_table_t dst, page_table_t src,
		     void *_vaddr, size_t size)
{
	struct paging_structures pg = {
		.root_paging = root_paging,
		.root_table = src,
	};
	page_table_t pt_dst[MAX_PAGE_TABLE_LEVELS];
	page_table_t pt_src[MAX_PAGE_TABLE_LEVELS];
	pt_entry_t pte_src, pte_dst;
	const struct paging *paging;
	unsigned long vaddr;
	unsigned int n;

	if (page_voffset(_vaddr))
		return -EINVAL;

	if (size % PAGE_SIZE)
		return -EINVAL;

	vaddr = (unsigned long)_vaddr;
	while (size > 0) {
		paging = pg.root_paging;
		n = 0;
		pt_src[n] = src;
		pt_dst[n] = dst;
		while (1) {
			pte_src = paging->get_entry(pt_src[n], vaddr);
			if (!paging->entry_valid(pte_src, PAGE_PRESENT_FLAGS))
				return -EINVAL;

			if ((vaddr & ~PMASK(paging->page_size)) == 0) {
				if (size >= paging->page_size) {
					pte_dst = paging->get_entry(pt_dst[n],
								    vaddr);
					*pte_dst = *pte_src;

					vaddr += paging->page_size;
					size -= paging->page_size;
					break;
				}
			}

			/* Walk deeper */
			pte_dst = paging->get_entry(pt_dst[n], vaddr);
			page_table_t next;
			if (paging->entry_valid(pte_dst, PAGE_PRESENT_FLAGS)) {
				next = p2v(paging->get_next_pt(pte_dst));
			} else {
				next = zalloc_pages(1);
				if (!next)
					return -ENOMEM;
				paging->set_next_pt(pte_dst, v2p(next));
			}
			pt_dst[n + 1] = next;
			pt_src[n + 1] = p2v(paging->get_next_pt(pte_src));
			n++;
			paging++;
		}
	}

	return 0;
}
