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

#include <grinch/csr.h>
#include <grinch/errno.h>
#include <grinch/irq.h>
#include <grinch/printk.h>
#include <grinch/sbi.h>
#include <grinch/sbi_handler.h>

int handle_ipi(void)
{
	return -ENOSYS;
}

int handle_timer(void)
{
	sbi_set_timer(-1); /* deactivate timer */

	return 0;
}

static int sbi_ext_time(struct sbiret *ret, unsigned int fid, u64 stime)
{
	if (fid != SBI_EXT_TIME_SET_TIMER) {
		printk("SBI_EXT_TIME: unknown fid: %x\n", fid);
		return -ENOSYS;
	}

	/* Clear pending IRQs */
	csr_clear(CSR_HVIP, (1 << IRQ_S_TIMER) << VSIP_TO_HVIP_SHIFT);

	/* simply forward the guest's timer wish */
	*ret = sbi_set_timer(stime);

	return 0;
}

int handle_ecall(struct registers *regs)
{
	unsigned int eid, fid;
	int err = -ENOSYS;
	struct sbiret ret = {
		.error = 0,
		.value = 0,
	};

	eid = regs->a7;
	fid = regs->a6;

	switch (eid) {
		case SBI_EXT_0_1_CONSOLE_PUTCHAR:
			sbi_console_putchar(regs->a0);
			err = 0;
			break;
		case SBI_EXT_TIME:
			err = sbi_ext_time(&ret, fid, regs->a0);
			break;
		default:
			printk("Unknown SBI EID: %x\n", eid);
			break;
	}

	if (!err) {
		/* we had an ecall, so skip 4b of instructions */
		regs->sepc += 4;
		regs->a0 = ret.error;
		regs->a1 = ret.value;
	}

	return err;
}
