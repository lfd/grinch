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

#define dbg_fmt(x)	"smp: " x

#include <asm/irq.h>
#include <asm/isa.h>
#include <asm/spinlock.h>

#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/init.h>
#include <grinch/paging.h>
#include <grinch/panic.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/string.h>

#include <grinch/arch/sbi.h>

/* Assembly entry point for secondary CPUs */
void secondary_start(void);

void arch_secondary_init(void)
{
	/* We still run on the shared boot root: switch to the kernel root */
	arch_paging_enable(this_cpu_id(), kernel_root);
}

void __init arch_smp_bringup_init(void)
{
	/*
	 * riscv has a single root: seed the boot root with the kernel
	 * mappings so secondaries can run virtually. smp_init() adds the
	 * identity trampoline on top.
	 */
	memcpy(secondary_boot_root, kernel_root, PAGE_SIZE);
}

int __init arch_boot_cpu(unsigned long hart_id)
{
	paddr_t paddr;
	struct sbiret ret;
	unsigned long opaque;
	struct per_cpu *pcpu;

	pr("Bringing up HART %lu\n", hart_id);
	pcpu = per_cpu(hart_id);

	pcpu->cpuid = hart_id;
	spin_init(&pcpu->remote_call.lock);

	paddr = v2p(secondary_start);

	/* Make it easy for secondary_entry: provide the content of satp */
	opaque = (v2p(secondary_boot_root) >> PAGE_SHIFT)
		| (csr_read(satp) & (SATP_MODE_MASK << SATP_MODE_SHIFT));

	ret = sbi_hart_start(hart_id, paddr, opaque);
	if (ret.error) {
		pr("Failed to bring up CPU %lu Error: %ld Value: %ld\n",
		   hart_id, ret.error, ret.value);
		return -ENOSYS;
	}

	return 0;
}


void ipi_send(unsigned long cpu)
{
	struct sbiret ret;

	ret = sbi_send_ipi((1UL << cpu), 0);
	if (ret.error != SBI_SUCCESS)
		pr("WARNING: Unable to send IPI\n");
}
