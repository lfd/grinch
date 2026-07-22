/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define ARCH_PER_CPU_FIELDS				\
	unsigned long mpidr;

#ifndef __ASSEMBLY__

#include <asm/sysregs.h>

#include <grinch/compiler_attributes.h>

struct per_cpu;

/*
 * TPIDR_EL1 holds the pointer to this CPU's per_cpu structure. It is set
 * once per CPU during early boot.
 */
static __always_inline struct per_cpu *this_per_cpu(void)
{
	void *ret;

	arm_read_sysreg(TPIDR_EL1, ret);
	return ret;
}

#endif /* !__ASSEMBLY__ */
