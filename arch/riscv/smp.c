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
#include <grinch/task.h>

#include <grinch/arch/sbi.h>

/* Assembly entry point for secondary CPUs */
void secondary_start(void);

/*
 * All secondary CPUs enter virtual addressing through one shared boot
 * root: the kernel mappings plus the identity-mapped trampoline. Each
 * CPU leaves it for its own root table as soon as it runs in virtual
 * space, so the boot root is only ever read.
 */
static page_table_t secondary_boot_root;

/* C entry point for secondary CPUs */
void secondary_cmain(void);

void secondary_cmain(void)
{
	irq_disable();
	ext_disable();
	ipi_disable();
	timer_disable();

	/* We still run on the shared boot root: switch to the kernel root */
	arch_paging_enable(this_cpu_id(), kernel_root);

	ipi_enable();
	bitmap_set(cpus_online, this_cpu_id(), 1);
	mb();

	/*
	 * We will enter idle here, and wait idle until we are kicked by
	 * another CPU
	 */
	prepare_user_return();
}

int __init arch_smp_bringup_init(void)
{
	paddr_t paddr;
	int err;

	secondary_boot_root = zalloc_pages(1);
	if (!secondary_boot_root)
		return -ENOMEM;

	memcpy(secondary_boot_root, kernel_root, PAGE_SIZE);

	/* The boot root must contain a boot trampoline */
	paddr = v2p(grinch_base());
	err = map_range(secondary_boot_root, (void *)paddr, paddr,
			GRINCH_SIZE, GRINCH_MEM_RX);
	if (err)
		return err;

	return 0;
}

int arch_boot_cpu(unsigned long hart_id)
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

/*
 * Once every secondary is online it has switched to the kernel root, so
 * the shared boot root is dead and can be released - along with the
 * private tables its identity trampoline pulled in, which do not belong
 * to the kernel root and would otherwise leak.
 */
void __init arch_smp_bringup_done(void)
{
	paddr_t paddr;
	int err;

	if (!secondary_boot_root)
		return;

	paddr = v2p(grinch_base());
	err = unmap_range(secondary_boot_root, (void *)paddr, GRINCH_SIZE);
	if (err)
		BUG();

	free_pages(secondary_boot_root, 1);
	secondary_boot_root = NULL;
}

void ipi_send(unsigned long cpu)
{
	struct sbiret ret;

	ret = sbi_send_ipi((1UL << cpu), 0);
	if (ret.error != SBI_SUCCESS)
		pr("WARNING: Unable to send IPI\n");
}
