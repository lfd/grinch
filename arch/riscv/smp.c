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

#include <asm/firmware.h>
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

/* Assembly entry point for secondary CPUs */
void secondary_start(void);

void arch_secondary_init(void)
{
#ifdef CONFIG_MMU
	/* We still run on the shared boot root: switch to the kernel root */
	arch_paging_enable(this_cpu_id(), kernel_root);
#endif
}

#ifdef CONFIG_MMU
void __init arch_smp_bringup_init(void)
{
	/*
	 * riscv has a single root: seed the boot root with the kernel
	 * mappings so secondaries can run virtually. smp_init() adds the
	 * identity trampoline on top.
	 */
	memcpy(secondary_boot_root, kernel_root, PAGE_SIZE);
}
#endif

int __init arch_boot_cpu(unsigned long hart_id)
{
	unsigned long opaque;
	struct per_cpu *pcpu;
	paddr_t paddr;

	pr("Bringing up HART %lu\n", hart_id);
	pcpu = per_cpu(hart_id);

	pcpu->cpuid = hart_id;
	spin_init(&pcpu->remote_call.lock);

	paddr = v2p(secondary_start);

#ifdef CONFIG_MMU
	/* Make it easy for secondary_entry: provide the content of satp */
	opaque = (v2p(secondary_boot_root) >> PAGE_SHIFT)
		| (csr_read(satp) & (SATP_MODE_MASK << SATP_MODE_SHIFT));
#else
	/* Nothing to hand over: the secondary runs where it lands. */
	opaque = 0;
#endif

	return firmware_hart_start(hart_id, paddr, opaque);
}


void ipi_send(unsigned long cpu)
{
	firmware_ipi_send(1UL << cpu);
}
