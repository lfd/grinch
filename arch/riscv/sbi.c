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

#define dbg_fmt(x) "sbi: " x

#include <grinch/errno.h>
#include <grinch/printk.h>

#include <grinch/arch/sbi.h>

static unsigned long sbi_spec_version;

static inline unsigned long __init sbi_major_version(void)
{
	return (sbi_spec_version >> SBI_SPEC_VERSION_MAJOR_SHIFT) &
		SBI_SPEC_VERSION_MAJOR_MASK;
}

static inline unsigned long __init sbi_minor_version(void)
{
	return sbi_spec_version & SBI_SPEC_VERSION_MINOR_MASK;
}

static bool __init sbi_probe_extension(unsigned long extension, const char *name)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_PROBE_EXT, extension, 0, 0, 0, 0, 0);
	if (!ret.error) {
		pr_info_i("SBI extension %s %savailable\n",
			  name, ret.value ? "" : "not ");
		return !!ret.value;
	}
	
	return 0;
}

int __init sbi_init(void)
{
	struct sbiret ret;
	bool ext;

	pr_info_i("Initialising SBI\n");
	
	ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_SPEC_VERSION, 0, 0, 0, 0, 0, 0);
	if (ret.error != SBI_SUCCESS) {
		pr_warn_i("Unable to get SBI version\n");
		return -ENOSYS;
	}

	sbi_spec_version = ret.value;
	pr_info_i("SBI version v%lu.%lu detected\n", sbi_major_version(), sbi_minor_version());

	if (sbi_major_version() == 0 && sbi_minor_version() <= 1) {
		pr_warn_i("SBI too old! Consider upgrading your firmware.\n");
		return -ENOSYS;
	}

	/* Following extensions are required for grinch to run */
	ext = sbi_probe_extension(SBI_EXT_TIME, ISTR("TIME"));
	if (!ext)
		return -ENOSYS;

	ext = sbi_probe_extension(SBI_EXT_RFENCE, ISTR("RFENCE"));
	if (!ext)
		return -ENOSYS;

	/* Those two extensions are required for SMP support */
	ext = sbi_probe_extension(SBI_EXT_IPI, ISTR("IPI"));
	if (!ext)
		return -ENOSYS;

	ext = sbi_probe_extension(SBI_EXT_HSM, ISTR("HSM"));
	if (!ext)
		return -ENOSYS;

	return 0;
}
