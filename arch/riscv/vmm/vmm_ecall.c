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

#define dbg_fmt(x)	"vmm ecall: " x

#include <asm/csr.h>

#include <grinch/console.h>
#include <grinch/errno.h>
#include <grinch/hypercall.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/serial.h>
#include <grinch/task.h>
#include <grinch/syscall.h>
#include <grinch/task.h>
#include <grinch/timer.h>

#include <grinch/arch/sbi.h>
#include <grinch/arch/vmm.h>

static inline struct sbiret handle_sbi_time(unsigned long fid, unsigned long a0)
{
	struct sbiret ret;

	switch (fid) {
		case SBI_EXT_TIME_SET_TIMER:
			current_task()->vmachine->vregs.hvip &= ~VIE_TIE;
			if (a0 != (unsigned long)-1)
				task_sleep_until(current_task(),
						 timer_ticks_to_time(a0));
			else
				task_cancel_timer(current_task());
			ret.error = 0;
			ret.value = 0;
			break;

		default:
			pr("Time FID %lx not implemented\n", fid);
			ret.error = SBI_ERR_NOT_SUPPORTED;
			ret.value = 0;
			break;
	}

	return ret;
}

static inline struct sbiret sbi_probe_extension(long eid)
{
	struct sbiret ret;

	ret.error = 0;
	switch (eid) {
		case SBI_EXT_TIME:
		case SBI_EXT_RFENCE: /* not implemented */
		case SBI_EXT_IPI: /* not implemented */
		case SBI_EXT_HSM: /* not implemented */
			ret.value = 1;
			break;

		default:
			ret.value = 0;
	}

	return ret;
}

static inline struct sbiret
handle_sbi_base(unsigned long fid, unsigned long arg0)
{
	struct sbiret ret;

	ret.error = SBI_SUCCESS;
	switch (fid) {
		case SBI_EXT_BASE_GET_SPEC_VERSION:
			ret.value = sbi_version(2, 0);
			break;

		case SBI_EXT_BASE_GET_IMP_ID:
			ret.value = 1;
			break;

		case SBI_EXT_BASE_GET_IMP_VERSION:
			ret.value = 0x10003;
			break;

		case SBI_EXT_BASE_PROBE_EXT:
			ret = sbi_probe_extension(arg0);
			break;

		case SBI_EXT_BASE_GET_MVENDORID:
		case SBI_EXT_BASE_GET_MARCHID:
		case SBI_EXT_BASE_GET_MIMPID:
			ret.value = 0;
			break;

		default:
			pr("Base FID %lx not implemented\n", fid);
			ret.error = SBI_ERR_NOT_SUPPORTED;
			ret.value = 0;
			break;
	}

	return ret;
}

static inline struct sbiret handle_sbi_grinch(unsigned long num, unsigned long arg0)
{
	struct sbiret ret;

	switch (num) {
		case GRINCH_HYPERCALL_PRESENT:
			ret.error = SBI_SUCCESS;
			ret.value = current_task()->pid;
			break;

		case GRINCH_HYPERCALL_BP:
			ret.error = SBI_SUCCESS;
			ret.value = 42;
			break;

		case GRINCH_HYPERCALL_YIELD:
			ret.error = SBI_SUCCESS;
			ret.value = 0;
			this_per_cpu()->schedule = true;
			break;

		case GRINCH_HYPERCALL_VMQUIT:
			task_exit(arg0);
			break;

		default:
			pr("Unknown Grinch Hypercall %lx\n", num);
			ret.error = SBI_ERR_NOT_SUPPORTED;
			ret.value = 0;
			break;
	}

	return ret;
}

int vmm_handle_ecall(void)
{
	struct registers *regs;
	unsigned long eid, fid;
	struct sbiret ret;
	char buf[2];

	regs = &current_task()->regs;
	regs->pc += 4;

	eid = regs->a7;
	fid = regs->a6;
	switch (eid) { /* EID - Extension ID*/
		case SBI_EXT_0_1_CONSOLE_PUTCHAR:
			buf[0] = regs->a0;
			buf[1] = 0;
			console_puts(buf);
			ret.error = ret.value = 0;
			break;

		case SBI_EXT_BASE:
			ret = handle_sbi_base(fid, regs->a0);
			break;

		case SBI_EXT_TIME:
			ret = handle_sbi_time(fid, regs->a0);
			break;

		case SBI_EXT_GRNC:
			ret = handle_sbi_grinch(fid, regs->a0);
			break;

		default:
			pr("Extension 0x%lx not implemented\n", eid);
			return -ENOSYS;
	}

	if (eid == SBI_EXT_GRNC && fid == GRINCH_HYPERCALL_VMQUIT)
		return 0;

	regs->a0 = ret.error;
	regs->a1 = ret.value;

	return 0;
}
