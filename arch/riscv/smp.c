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
	unsigned int index;
	int err;

	pr("Bringing up HART %lu\n", hart_id);
	pcpu = per_cpu(hart_id);

	/* Zero everything */
	memset(pcpu, 0, sizeof(struct per_cpu));

	pcpu->cpuid = hart_id;
	spin_init(&pcpu->remote_call.lock);

	/* Copy over kernel page tables */

	/*
	 * FIXME: This is done by handwork at the moment, and only works with
	 * SV39. This must be improved, if we ever use some other pagers
	 */

	/* Hook in the whole kernel. */
	index = vaddr2vpn(_load_addr, satp_mode == SATP_MODE_39 ? 2 : 3);
	pcpu->root_table_page[index] = this_root_table_page()[index];

	/* Hook in the direct physical area */
	index = vaddr2vpn((void *)DIR_PHYS_BASE, satp_mode == SATP_MODE_39 ? 2 : 3);
	pcpu->root_table_page[index] = this_root_table_page()[index];

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
		| satp_mode;

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

int __init platform_init(void)
{
	const char *name, *isa;
	unsigned long hart_id;
	int err, cpu, child;
	const fdt32_t *reg;

	bitmap_set(cpus_online, this_cpu_id(), 1);

	cpu = fdt_path_offset(_fdt, "/cpus");
	if (cpu < 0) {
		pri("No CPUs found in device-tree. Halting.\n");
		return -ENOSYS;
	}

	fdt_for_each_subnode(child, _fdt, cpu) {
		name = fdt_get_name(_fdt, child, NULL);
		if (strcmp(name, "cpu-map") == 0)
			continue;

		reg = fdt_getprop(_fdt, child, "reg", &err);
		if (err < 0) {
			pri("%s: Error reading reg\n", name);
			return -EINVAL;
		}
		hart_id = fdt32_to_cpu(reg[0]);
		if (hart_id >= MAX_CPUS) {
			pri("%s: HART %lu beyond MAX_CPUS\n", name, hart_id);
			return -ERANGE;
		}

		isa = fdt_getprop(_fdt, child, "riscv,isa", &err);
		if (!isa || err < 0) {
			pri("CPU %lu: No ISA specification found\n", hart_id);
		} else {
			pri("CPU %lu: Found ISA level: %s\n", hart_id, isa);
		}
		err = riscv_isa_update(hart_id, isa);
		if (err)
			return err;

		if (!fdt_device_is_available(_fdt, child)) {
			pri("%s: HART %lu disabled via device-tree\n", name,
			   hart_id);
			continue;
		}

		bitmap_set(cpus_available, hart_id, 1);

		pri("%s: HART %lu available\n", name, hart_id);

		err = phys_mark_used(v2p(per_cpu(hart_id)),
				     PAGES(sizeof(struct per_cpu)));
		if (err)
			return err;
	}

	return 0;
}
