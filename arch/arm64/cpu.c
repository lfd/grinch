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

#include <asm/cpu.h>
#include <grinch/cpu.h>
#include <grinch/hypercall.h>
#include <grinch/paging.h>
#include <grinch/panic.h>

bool grinch_is_guest;

void dump_exception(struct trap_context *ctx) { for (;;); }
void dump_regs(struct registers *a) { for (;;); }
int hypercall(unsigned long no, unsigned long arg1) { BUG(); }

/* Flush an entire address space on every CPU, including this one. */
void flush_tlb_asid(unsigned long asid)
{
	/* aside1is broadcasts across the inner-shareable domain. */
	asm volatile(
		"dsb ishst\n\t"
		"tlbi aside1is, %0\n\t"
		"dsb ish\n\t"
		"isb\n\t"
		: : "r" (asid << 48) : "memory");
}

/*
 * arm64 TLB maintenance broadcasts across the inner-shareable domain
 * (tlbi ...is), so translations on other CPUs are already invalidated by
 * the local unmap. Nothing to shoot down separately.
 */
void flush_tlb_others_asid(unsigned long asid, const void *addr, size_t size)
{
}
