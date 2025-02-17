/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/compiler_attributes.h>
#include <grinch/errno.h>
#include <grinch/header.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/symbols.h>
#include <grinch/string.h>

/* Set PTE access bits to RWX + AD to prevent page faults */
#define PAGE_FLAGS_DEFAULT				\
	(PAGE_PRESENT_FLAGS | RISCV_PTE_FLAG(R) |	\
	 RISCV_PTE_FLAG(W) | RISCV_PTE_FLAG(X) |	\
	 RISCV_PTE_FLAG(A) | RISCV_PTE_FLAG(D))

void __noreturn
loader(unsigned long hart_id, paddr_t fdt, paddr_t load_addr);

void __noreturn
grinch_start(unsigned long hart_id, paddr_t fdt, paddr_t dst);

static void __init *loader_page_zalloc(void **next)
{
	void *tmp;

	tmp = *next;
	memset(tmp, 0, PAGE_SIZE);
	*next += PAGE_SIZE;

	return tmp;
}

/* 2 MiB page size, in case of RV64 (SV39) */
#if ARCH_RISCV == 32 /* rv32 */
static inline __init void enable_mmu(paddr_t l0)
{
	enable_mmu_satp(SATP_MODE_32, l0);
}

static void __init
map_mega(void **next, unsigned long *l0, void *vaddr, paddr_t paddr)
{
	unsigned long *l0_entry;

	l0_entry = &l0[vaddr2vpn(vaddr, 1)];
	*l0_entry = paddr2pte(paddr) | PAGE_FLAGS_DEFAULT;
}
#elif ARCH_RISCV == 64 /* rv64 */
static void __init
map_mega(void **next, unsigned long *l0, void *vaddr, paddr_t paddr)
{
	unsigned long *l0_entry, *l1_entry, *l1;

	l0_entry = &l0[vaddr2vpn(vaddr, 2)];
	if (*l0_entry & RISCV_PTE_FLAG(V)) {
		l1 = (unsigned long *)pte2table(*l0_entry);
	} else {
		l1 = loader_page_zalloc(next);
		*l0_entry = paddr2pte((paddr_t)l1) | PAGE_PRESENT_FLAGS;
	}

	l1_entry = &l1[vaddr2vpn(vaddr, 1)];
	*l1_entry = paddr2pte(paddr) | PAGE_FLAGS_DEFAULT;
}

/* On RV64, we will use the SV39 paging system for handover */
static inline void __init enable_mmu(paddr_t l0)
{
	enable_mmu_satp(SATP_MODE_39, l0);
}
#endif /* rv64 */

void __noreturn __init
loader(unsigned long hart_id, paddr_t fdt, paddr_t load_addr)
{
	void *next, *l0;
	unsigned int d;
	paddr_t offset;

	/*
	 * Place temporary page tables at the end of the internal page pool.
	 * For the kernel's initial page tables grinch will use the internal
	 * page pool, so this is fine.
	 */
	next = (void*)load_addr + GRINCH_SIZE;
	l0 = loader_page_zalloc(&next);
	for (d = 0; d < mega_page_up(num_os_pages() * PAGE_SIZE);
	     d+= MEGA_PAGE_SIZE) {
		/* ID map loaded location */
		map_mega(&next, l0, (void*)load_addr + d, load_addr + d);
		/* linked location of bootloader */
		map_mega(&next, l0, __start + d, load_addr + d);
	}

	enable_mmu((paddr_t)l0);
	offset = (paddr_t)__start - load_addr;
	asm volatile(
		"add sp, sp, %[offset]\n"
		"la a0, virt_start\n"
		"jr a0\n"
		"virt_start:"
		: : [offset] "r"(offset) : "a0");

	grinch_start(hart_id, fdt, load_addr);
}
