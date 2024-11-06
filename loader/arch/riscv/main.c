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
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/symbols.h>
#include <grinch/string.h>
#include <grinch/loader.h>
#include <grinch/vmgrinch_header.h>

/* Set PTE access bits to RWX + AD to prevent page faults */
#define PAGE_FLAGS_DEFAULT				\
	(PAGE_PRESENT_FLAGS | RISCV_PTE_FLAG(R) |	\
	 RISCV_PTE_FLAG(W) | RISCV_PTE_FLAG(X) |	\
	 RISCV_PTE_FLAG(A) | RISCV_PTE_FLAG(D))

extern unsigned char __start[];
extern unsigned char __stack_end[];

void __noreturn
loader(unsigned long hart_id, paddr_t fdt, paddr_t load_addr);

static void *loader_page_zalloc(void **next)
{
	void *tmp;

	tmp = *next;
	memset(tmp, 0, PAGE_SIZE);
	*next += PAGE_SIZE;

	return tmp;
}

/* 2 MiB page size, in case of RV64 (SV39) */
static void map_mega(void **next, unsigned long *l0, void *vaddr, paddr_t paddr)
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
static inline void enable_mmu(paddr_t l0)
{
	enable_mmu_satp(SATP_MODE_39, l0);
}

void __noreturn
loader(unsigned long hart_id, paddr_t fdt, paddr_t load_addr)
{
	void __noreturn (*grinch_entry)
		(unsigned long hart_id, paddr_t fdt, paddr_t dst);
	paddr_t p_grinch_dst;
	void *next, *l0;
	unsigned int d;
	paddr_t offset;

	next = __stack_end - __start + (void*)load_addr;
	l0 = loader_page_zalloc(&next);
	for (d = 0; d < mega_page_up(__stack_end - __start);
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

	/* Alright, here we have a working environment */
	p_grinch_dst = load_addr + mega_page_up(__stack_end - __start);
	for (d = 0; d < GRINCH_SIZE; d += MEGA_PAGE_SIZE)
		map_mega(&next, l0, (void*)VMGRINCH_BASE + d, p_grinch_dst + d);

	loader_copy_grinch();

	grinch_entry = ((struct vmgrinch_header *)VMGRINCH_BASE)->entry;
	grinch_entry(hart_id, fdt, p_grinch_dst);
}
