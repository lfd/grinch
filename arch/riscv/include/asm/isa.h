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

#ifndef _ISA_H

#include <grinch/percpu.h>
#include <grinch/init.h>

#define RISCV_ISA_HYPERVISOR	(1 << 0)

#ifndef __ASSEMBLY__

typedef unsigned int riscv_isa_t;

extern riscv_isa_t riscv_isa;

static inline bool has_hypervisor(void)
{
	return riscv_isa & RISCV_ISA_HYPERVISOR;
}

int riscv_isa_update(unsigned long hart, const char *string);

#endif /* __ASSEMBLY__ */

#endif /* _ISA_H */
