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

#define dbg_fmt(x) "sbi: " x

#include <grinch/printk.h>
#include <grinch/sbi.h>

#define SBI_EXT_BASE			0x10
#define SBI_EXT_BASE_GET_SPEC_VERSION	0

#define SBI_SPEC_VERSION_DEFAULT	0x1
#define SBI_SPEC_VERSION_MAJOR_SHIFT	24
#define SBI_SPEC_VERSION_MAJOR_MASK	0x7f
#define SBI_SPEC_VERSION_MINOR_MASK	0xffffff

static unsigned long sbi_spec_version;

static long __sbi_base_ecall(int fid)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_BASE, fid, 0, 0, 0, 0, 0, 0);
	if (!ret.error)
		return ret.value;
	else {
		pr("SBI Error %lx when executing SBI_EXT_BASE + %x\n", ret.error, fid);
		return -1;
	}
}

static inline unsigned long sbi_major_version(void)
{
	return (sbi_spec_version >> SBI_SPEC_VERSION_MAJOR_SHIFT) &
		SBI_SPEC_VERSION_MAJOR_MASK;
}

static inline unsigned long sbi_minor_version(void)
{
	return sbi_spec_version & SBI_SPEC_VERSION_MINOR_MASK;
}


static inline long sbi_get_spec_version(void)
{
	return __sbi_base_ecall(SBI_EXT_BASE_GET_SPEC_VERSION);
}

int sbi_init(void)
{
	ps("Initialising SBI\n");

	sbi_spec_version = sbi_get_spec_version();
	pr("SBI version v%lu.%lu detected\n", sbi_major_version(), sbi_minor_version());

	if (sbi_major_version() == 0 && sbi_minor_version() <= 1) {
		ps("SBI too old! Consider upgrading your firmware.\n");
		return -1;
	}

	return 0;
}
