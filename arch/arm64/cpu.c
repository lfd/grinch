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
#include <asm/irq.h>
#include <asm/sysregs.h>

#include <grinch/cpu.h>
#include <grinch/hypercall.h>
#include <grinch/paging.h>
#include <grinch/panic.h>
#include <grinch/printk.h>

bool grinch_is_guest;

static const char *exception_codes[] = {
	[ESR_EC_UNKNOWN] = "Unknown",
	[ESR_EC_SVC64]	= "SVC64",
	[ESR_EC_IABT_LOW] = "IABT low",
	[ESR_EC_DABT_LOW] = "DABT low",
};

void dump_exception(struct trap_context *ctx)
{
	const char *cause_str;
	u32 ec;

	ec = ESR_EC(ctx->esr);
	if (ec < ARRAY_SIZE(exception_codes))
		cause_str = exception_codes[ec];
	else
		cause_str = exception_codes[0];

	pr("Exception Code: 0x%x (%s)\n", ec, cause_str);
}

void dump_regs(struct registers *a)
{
	unsigned int x;

	pr("Context:\n");
	pr(" PC: %016lx  SP: %016lx SPSR: %016lx\n", a->pc, a->sp, a->spsr);
	pr(" LR: %016lx  ELR: %016lx\n", a->usr[30], a->elr);

	pr("Registers:\n");
	for (x = 0; x < NUM_REGS - 1; x+= 3) {
		pr("%sX%u: %016lx %sX%u: %016lx %sX%u: %016lx\n",
		   (x + 0 < 10) ? " " : "", x + 0, a->usr[x + 0],
		   (x + 1 < 10) ? " " : "", x + 1, a->usr[x + 1],
		   (x + 2 < 10) ? " " : "", x + 2, a->usr[x + 2]);
	}
}

int hypercall(unsigned long no, unsigned long arg1)
{
	BUG();
}

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
