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

#define dbg_fmt(x) "sbi: " x

#include <asm/firmware.h>

#include <grinch/errno.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/reboot.h>

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

void firmware_timer_set(u64 ticks)
{
	struct sbiret ret;

	ret = sbi_set_timer(ticks);
	if (ret.error)
		panic("SBI Error\n");
}

void firmware_ipi_send(unsigned long hmask)
{
	struct sbiret ret;

	ret = sbi_send_ipi(hmask, 0);
	if (ret.error != SBI_SUCCESS)
		pr("WARNING: Unable to send IPI\n");
}

int firmware_hart_start(unsigned long hart_id, paddr_t entry,
			unsigned long opaque)
{
	struct sbiret ret;

	ret = sbi_hart_start(hart_id, entry, opaque);
	if (ret.error) {
		pr("Failed to start hart %lu Error: %ld Value: %ld\n",
		   hart_id, ret.error, ret.value);
		return -ENOSYS;
	}

	return 0;
}

void firmware_remote_fence(unsigned long hmask, const void *addr, size_t size)
{
	struct sbiret ret;

	ret = sbi_rfence_sfence_vma(hmask, 0, (unsigned long)addr, size);
	if (ret.error != SBI_SUCCESS)
		BUG();
}

void firmware_remote_fence_asid(unsigned long hmask, unsigned long asid,
				const void *addr, size_t size)
{
	struct sbiret ret;

	ret = sbi_rfence_sfence_vma_asid(hmask, 0, (unsigned long)addr, size,
					 asid);
	if (ret.error != SBI_SUCCESS)
		BUG();
}

static int sbi_shutdown(int err)
{
	sbi_system_reset(SBI_SRST_RESET_TYPE_SHUTDOWN,
			 SBI_SRST_RESET_REASON_NONE);
	return -EIO;
}

static int sbi_reboot(void)
{
	sbi_system_reset(SBI_SRST_RESET_TYPE_COLD_REBOOT,
			 SBI_SRST_RESET_REASON_NONE);
	return -EIO;
}

int __init firmware_init(void)
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

	/* Optional: system reset. Leave whatever above us already provides. */
	if (sbi_probe_extension(SBI_EXT_SRST, ISTR("SRST"))) {
		if (!arch_shutdown)
			arch_shutdown = sbi_shutdown;
		if (!arch_reboot)
			arch_reboot = sbi_reboot;
	}

	return 0;
}

