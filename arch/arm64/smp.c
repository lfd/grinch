/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"smp: " x

#include <asm/irq.h>
#include <asm/psci.h>
#include <asm/spinlock.h>
#include <asm/sysregs.h>

#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/init.h>
#include <grinch/irqchip.h>
#include <grinch/paging.h>
#include <grinch/panic.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/string.h>

/* Assembly entry point for secondary CPUs (head.S). */
void secondary_start(void);

/*
 * MMU configuration consumed by secondary_start while the MMU is still off
 * (read physically, PC-relative). The boot CPU captures its own running
 * configuration so secondaries come up bit-for-bit identical.
 *
 * secondary_start() loads these as a contiguous block; keep the field
 * order in sync with the load sequence in head.S.
 */
struct secondary_mmu_config {
	u64 ttbr0;
	u64 ttbr1;
	u64 mair;
	u64 tcr;
	u64 sctlr;
};
static_assert(sizeof(struct secondary_mmu_config) == 5 * sizeof(u64));

struct secondary_mmu_config secondary_mmu;

void arch_secondary_init(void)
{
}

void ipi_send(unsigned long cpu_id)
{
	irqchip_send_ipi(cpu_id);
}

int __init arch_boot_cpu(unsigned long cpu_id)
{
	struct per_cpu *pcpu = per_cpu(cpu_id);
	const char *method;
	int err;

	pcpu->cpuid = cpu_id;
	spin_init(&pcpu->remote_call.lock);

	/* The CPU node's enable-method selects the bring-up conduit. */
	method = fdt_getprop(_fdt, pcpu->of_node, ISTR("enable-method"), &err);
	if (!method) {
		pr("CPU %lu: no enable-method in device-tree\n", cpu_id);
		return -ENODEV;
	}

	if (strcmp(method, ISTR("psci")) == 0)
		return psci_cpu_on(pcpu->mpidr, v2p(secondary_start), 0);

	pr("CPU %lu: unsupported enable-method '%s'\n", cpu_id, method);
	return -ENOSYS;
}

void __init arch_smp_bringup_init(void)
{
	/*
	 * Capture the running MMU configuration for the secondaries. They
	 * replay it verbatim, so TTBR1/MAIR/TCR (and the MMU-on SCTLR) match
	 * the boot CPU exactly; TTBR0 is the identity trampoline that
	 * smp_init() maps into secondary_boot_root.
	 */
	secondary_mmu.ttbr0 = v2p(secondary_boot_root);
	secondary_mmu.ttbr1 = v2p(kernel_root);
	arm_read_sysreg(MAIR, secondary_mmu.mair);
	arm_read_sysreg(TCR, secondary_mmu.tcr);
	/*
	 * The kernel runs with TTBR0 walks disabled (EPD0=1), but secondaries
	 * enter through the TTBR0 identity trampoline. Keep TTBR0 live for the
	 * MMU-enable; head.S re-sets EPD0 once running virtually.
	 */
	secondary_mmu.tcr &= ~TCR_EL1_EPD0;
	arm_read_sysreg(SCTLR, secondary_mmu.sctlr);
	mb();
}
