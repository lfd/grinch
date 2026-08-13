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

#ifndef _ASM_HYPERCALL_H
#define _ASM_HYPERCALL_H

#include <grinch/errno.h>

#include <grinch/arch/sbi.h>

/* The hypervisor picks up grinch hypercalls as a vendor SBI extension. */
static inline int hypercall(unsigned long no, unsigned long arg1)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_GRNC, no, arg1, 0, 0, 0, 0, 0);
	if (ret.error != SBI_SUCCESS)
		return -EINVAL;

	return ret.value;
}

#endif /* _ASM_HYPERCALL_H */
