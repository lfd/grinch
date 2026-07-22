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

#define dbg_fmt(x)	"psci: " x

#include <asm/psci.h>

#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/init.h>
#include <grinch/printk.h>
#include <grinch/string.h>

/* Standard PSCI 0.2+ function ids (CPU_ON uses the SMC64 convention). */
#define PSCI_0_2_FN_PSCI_VERSION	0x84000000
#define PSCI_0_2_FN64_CPU_ON		0xc4000003

/* The discovered conduit, or NULL if no PSCI interface is present. */
static long (*psci_fn)(unsigned long fn, unsigned long a1, unsigned long a2,
		       unsigned long a3);
static u32 psci_version;

#define PSCI_FN(conduit)						\
static long psci_fn_##conduit(unsigned long fn, unsigned long a1,	\
			      unsigned long a2, unsigned long a3)	\
{									\
	register unsigned long x0 asm("x0") = fn;			\
	register unsigned long x1 asm("x1") = a1;			\
	register unsigned long x2 asm("x2") = a2;			\
	register unsigned long x3 asm("x3") = a3;			\
									\
	asm volatile(#conduit " #0"					\
		     : "+r" (x0), "+r" (x1), "+r" (x2), "+r" (x3)	\
		     : : "memory");					\
									\
	return x0;							\
}

PSCI_FN(hvc)
PSCI_FN(smc)

int psci_cpu_on(unsigned long mpidr, paddr_t entry, unsigned long context)
{
	long ret;

	if (!psci_fn)
		return -ENODEV;

	ret = psci_fn(PSCI_0_2_FN64_CPU_ON, mpidr, entry, context);
	if (ret < 0) {
		pr("CPU_ON(MPIDR 0x%lx) failed: %ld\n", mpidr, ret);
		return -EIO;
	}

	return 0;
}

void __init psci_init(void)
{
	const char *method;
	int off, err;

	off = fdt_path_offset(_fdt, "/psci");
	if (off < 0) {
		pri("No PSCI node in device-tree\n");
		return;
	}

	method = fdt_getprop(_fdt, off, ISTR("method"), &err);
	if (!method || err < 0) {
		pri("PSCI node without method\n");
		return;
	}

	if (strcmp(method, ISTR("hvc")) == 0) {
		psci_fn = psci_fn_hvc;
	} else if (strcmp(method, ISTR("smc")) == 0) {
		psci_fn = psci_fn_smc;
	} else {
		pri("Unknown PSCI method '%s'\n", method);
		return;
	}

	psci_version = psci_fn(PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0);

	pri("Detected PSCI %u.%u (%s conduit)\n", psci_version >> 16,
	    psci_version & 0xffff, method);
}
