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

#include <asm_generic/grinch_layout.h>

#include <grinch/compiler_attributes.h>
#include <grinch/errno.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/symbols.h>
#include <grinch/loader.h>

/* Set PTE access bits to RWX + AD to prevent page faults */
#define PAGE_FLAGS_DEFAULT				\
	(PAGE_PRESENT_FLAGS | RISCV_PTE_FLAG(R) |	\
	 RISCV_PTE_FLAG(W) | RISCV_PTE_FLAG(X) |	\
	 RISCV_PTE_FLAG(A) | RISCV_PTE_FLAG(D))

extern unsigned char __start[];
extern unsigned char __stack_end[];

void __noreturn
loader(unsigned long hart_id, paddr_t fdt, paddr_t load_addr, void *vbase);

static void *loader_page_zalloc(void **next)
{
	void *tmp;

	tmp = *next;
	memset(tmp, 0, PAGE_SIZE);
	*next += PAGE_SIZE;

	return tmp;
}

static void map_2M(void **next, u64 *l0, void *vaddr, paddr_t paddr)
{
	u64 *l0_entry = &l0[vaddr2vpn(vaddr, 2)];
	u64 *l1_entry;
	u64 *l1;

	if (*l0_entry & RISCV_PTE_FLAG(V)) {
		l1 = (u64*)pte2table(*l0_entry);
	} else {
		l1 = loader_page_zalloc(next);
		*l0_entry = paddr2pte((paddr_t)l1) | PAGE_PRESENT_FLAGS;
	}

	l1_entry = &l1[vaddr2vpn(vaddr, 1)];
	*l1_entry = paddr2pte(paddr) | PAGE_FLAGS_DEFAULT;
}

void __noreturn
loader(unsigned long hart_id, paddr_t fdt, paddr_t load_addr, void *vbase)
{
	void __noreturn (*grinch_entry)
		(unsigned long hart_id, paddr_t fdt, u64 offset);
	void *next, *l0;
	unsigned int d;
	paddr_t offset;

	next = __stack_end - __start + (void*)load_addr;
	l0 = loader_page_zalloc(&next);
	for (d = 0; d < mega_page_up(__stack_end - __start);
	     d+= MEGA_PAGE_SIZE) {
		/* ID map loaded location */
		map_2M(&next, l0, (void*)load_addr + d, load_addr + d);
		/* linked location of bootloader */
		map_2M(&next, l0, __start + d, load_addr + d);
	}

	enable_mmu_satp(SATP_MODE_39, (paddr_t)l0);
	offset = (paddr_t)__start - load_addr;
	asm volatile(
		"add sp, sp, %[offset]\n"
		"la a0, virt_start\n"
		"jr a0\n"
		"virt_start:"
		: : [offset] "r"(offset) : "a0");

	/* Alright, here we have a working environment */
	u64 p_grinch_dst = load_addr + mega_page_up(__stack_end - __start);
	for (d = 0; d < GRINCH_SIZE; d += MEGA_PAGE_SIZE)
		map_2M(&next, l0, (void*)VMGRINCH_BASE + d, p_grinch_dst + d);

	loader_copy_grinch();

	grinch_entry = (void*)VMGRINCH_BASE;
	grinch_entry(hart_id, fdt, VMGRINCH_BASE - p_grinch_dst);
}
