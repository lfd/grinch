/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ASM_PERCPU_H
#define _ASM_PERCPU_H

#include <grinch/compiler_attributes.h>

#define ARCH_PER_CPU_FIELDS				\
	struct {					\
		/*					\
		 * Offset of interrupt-controller	\
		 * phandle node in device-tree.		\
		 */					\
		int cpu_phandle;			\
		u16 ctx;				\
	} plic;

#ifndef __ASSEMBLY__

struct per_cpu;

/*
 * tp holds the pointer to this CPU's per_cpu structure. It is set once
 * per CPU during early boot and reclaimed on every entry from user
 * space.
 */
static __always_inline struct per_cpu *this_per_cpu(void)
{
	void *ret;

	asm("mv %0, tp" : "=r"(ret));
	return ret;
}

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_PERCPU_H */
