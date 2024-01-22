/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/csr.h>
#include <asm/irq.h>

#include <grinch/errno.h>
#include <grinch/printk.h>

#include <grinch/arch/sbi.h>

int handle_ipi(void)
{
	return -ENOSYS;
}

int arch_handle_timer(void)
{
	sbi_set_timer(-1); /* deactivate timer */

	/* inject timer to VS */
	csr_set(CSR_HVIP, (1 << IRQ_S_TIMER) << VSIP_TO_HVIP_SHIFT);

	return 0;
}
