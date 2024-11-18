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

#define dbg_fmt(x)	"isa: " x

#include <asm/isa.h>

#include <grinch/errno.h>
#include <grinch/printk.h>
#include <grinch/string.h>

static __initdata bool isa_seen;
riscv_isa_t riscv_isa;

static riscv_isa_t
riscv_parse_isa_token(unsigned long hart_id, const char *token)
{
#if ARCH_RISCV == 64
	if (!strncmp(token, "rv64", 4)) {
#elif ARCH_RISCV == 32
	if (!strncmp(token, "rv32", 4)) {
#endif
		if (strchr(token, 'h')) {
			return RISCV_ISA_HYPERVISOR;
			pr("CPU %lu: Hypervisor extension detected\n",
			   hart_id);
		}
	}

	return 0;
}

int __init riscv_isa_update(unsigned long hart_id, const char *_isa)
{
	char *token, *isa_str;
	riscv_isa_t this_isa;
	char tmp[256];

	isa_str = tmp;
	strncpy(tmp, _isa, sizeof(tmp) - 1);

	this_isa = 0;
	while ((token = strsep(&isa_str, "_")))
		this_isa |= riscv_parse_isa_token(hart_id, token);

	if (isa_seen) {
		if ((riscv_isa & this_isa) != riscv_isa) {
			pr("Error: different ISA extensions across CPUs!\n");
			return -EINVAL;
		}
	} else {
		isa_seen = true;
		riscv_isa = this_isa;
	}

	return 0;
}


