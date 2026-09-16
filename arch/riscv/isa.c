/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"isa: " x

#include <asm/csr.h>
#include <asm/isa.h>

#include <grinch/errno.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/string.h>

static __initdata bool isa_seen;
riscv_isa_t riscv_isa;

#ifdef CONFIG_RISCV_M_MODE

bool riscv_have_umode = true;
static bool umode_probed;

/*
 * Ask the hart whether it implements user mode. misa says so directly, but is
 * allowed to read as zero when it reports nothing at all. Then fall back on
 * asking mstatus.MPP for user mode: the field is WARL, so a hart that has none
 * leaves a level behind that it does have.
 *
 * Every hart asks for itself, and all must give the same answer.
 */
void __init riscv_umode_probe(void)
{
	unsigned long misa, status;
	bool have;

	misa = csr_read(misa);
	if (misa) {
		have = !!(misa & MISA_U);
	} else {
		status = csr_read(CSR_STATUS);
		csr_clear(CSR_STATUS, SR_PP);
		have = !(csr_read(CSR_STATUS) & SR_PP);
		csr_write(CSR_STATUS, status);
	}

	if (umode_probed) {
		if (have != riscv_have_umode)
			panic("Harts disagree on user mode\n");
		return;
	}
	umode_probed = true;
	riscv_have_umode = have;

	if (!have)
		pri("No user mode: tasks run in machine mode, unprotected\n");
}

#endif /* CONFIG_RISCV_M_MODE */

static riscv_isa_t
riscv_parse_isa_token(unsigned long hart_id, const char *token)
{
#ifdef CONFIG_ARCH_RISCV64
	if (!strncmp(token, "rv64", 4)) {
#elif CONFIG_ARCH_RISCV32
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


