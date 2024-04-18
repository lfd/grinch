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

#define dbg_fmt(x)	"smp: " x

#include <asm/irq.h>
#include <asm/isa.h>
#include <asm/spinlock.h>

#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/task.h>

#include <grinch/arch/sbi.h>

/* Assembly entry point for secondary CPUs */
void secondary_start(void);

/* C entry point for secondary CPUs */
int secondary_cmain(struct registers *regs);

int secondary_cmain(struct registers *regs)
{
	int err;

	irq_disable();
	ext_disable();
	ipi_disable();
	timer_disable();

	/* Unmap bootstrap page tables */
	err = unmap_range(this_root_table_page(),
			  (void *)v2p(__load_addr), GRINCH_SIZE);
	if (err)
		goto out;

	ipi_enable();
	bitmap_set(cpus_online, this_cpu_id(), 1);
	mb();

	/*
	 * We will enter idle here, and wait idle until we are kicked by
	 * another CPU
	 */
	prepare_user_return();

out:
	if (err)
		pr("Unable to bring up CPU %lu\n", this_cpu_id());
	return err;
}

int arch_boot_cpu(unsigned long hart_id)
{
	paddr_t paddr;
	struct sbiret ret;
	unsigned long opaque;
	struct per_cpu *pcpu;
	int err;

	pr("Bringing up HART %lu\n", hart_id);
	pcpu = per_cpu(hart_id);

	pcpu->cpuid = hart_id;
	spin_init(&pcpu->remote_call.lock);

	/* Duplicate kernel page tables */

	/* Hook in the whole kernel. */
	err = paging_duplicate(pcpu->root_table_page, this_root_table_page(),
			       (void *)VMGRINCH_BASE, GIGA_PAGE_SIZE);
	if (err)
		return err;

	err = paging_duplicate(pcpu->root_table_page,
			       this_root_table_page(),
			       (void *)DIR_PHYS_BASE,
			       giga_page_up(memory_size()));
	if (err)
		return err;

	/*
	 * Hook in the percpu stack. Take care, SV48 is not supported, Stack
	 * addresses will overlap with the top-most page table!
	 */
	err = paging_cpu_init(hart_id);
	if (err)
		return err;

	/* The page table must contain a boot trampoline */
	paddr = v2p(__load_addr);
	map_range(pcpu->root_table_page, (void*)paddr, paddr, GRINCH_SIZE,
		  GRINCH_MEM_RX);

	paddr = v2p(secondary_start);

	/* Make it easy for secondary_entry: provide the content of satp */
	opaque = (v2p(per_cpu(hart_id)->root_table_page) >> PAGE_SHIFT)
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
