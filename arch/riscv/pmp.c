/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"pmp: " x

#include <asm/csr.h>
#include <asm/pmp.h>

#include <grinch/gfp.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/symbols.h>

/*
 * Whether this hart implements PMP at all: no CSR names it, so ask one of
 * them and catch the refusal. The trap vector is borrowed for a single
 * instruction; interrupts are still masked this early.
 */
static bool pmp_probe(void)
{
	unsigned long old, tmp, has;

	asm volatile(
		"csrr	%[old], " __stringify(CSR_TVEC) "\n"
		"la	%[tmp], 1f\n"
		"csrw	" __stringify(CSR_TVEC) ", %[tmp]\n"
		"li	%[has], 1\n"
		"csrr	%[tmp], pmpcfg0\n"
		"j	2f\n"
		".align	2\n"
		"1:	csrr	%[tmp], " __stringify(CSR_EPC) "\n"
		"addi	%[tmp], %[tmp], 4\n"
		"csrw	" __stringify(CSR_EPC) ", %[tmp]\n"
		"li	%[has], 0\n"
		"mret\n"
		"2:	csrw	" __stringify(CSR_TVEC) ", %[old]\n"
		: [old] "=&r"(old), [tmp] "=&r"(tmp), [has] "=&r"(has)
		: : "memory");

	return has;
}

void pmp_init(void)
{
	static bool announced;
	paddr_t kstart, kend;

	if (!pmp_probe()) {
		/*
		 * Absent any implemented entry, the levels below pass by
		 * default: nothing to do is exactly right.
		 */
		if (!announced) {
			announced = true;
			pri("No PMP implemented\n");
		}
		return;
	}

	/*
	 * Three regions, top of range against top of range: everything below
	 * the kernel, the kernel, and everything above it. The levels below
	 * machine mode may reach all that is not the kernel's; entries left
	 * unlocked do not bind machine mode itself.
	 */
	kstart = v2p(__start);
	kend = kstart + kernel_pages() * PAGE_SIZE;

	csr_write(pmpaddr0, kstart >> 2);
	csr_write(pmpaddr1, kend >> 2);
	csr_write(pmpaddr2, -1UL);
	csr_write(pmpcfg0, (PMP_TOR | PMP_X | PMP_W | PMP_R) |
			   PMP_TOR << 8 |
			   (PMP_TOR | PMP_X | PMP_W | PMP_R) << 16);
}
