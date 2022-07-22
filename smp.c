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

#define dbg_fmt(x)	"smp: " x

#include <grinch/bitmap.h>
#include <grinch/errno.h>
#include <grinch/irq.h>
#include <grinch/fdt.h>
#include <grinch/irq.h>
#include <grinch/mm.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/sbi.h>
#include <grinch/spinlock.h>
#include <grinch/smp.h>

#include "config.h"

#define for_each_cpu(cpu, set)  for_each_cpu_except(cpu, set, -1)
#define for_each_cpu_except(cpu, set, exception)                \
	for ((cpu) = -1;                                        \
	     (cpu) = next_cpu((cpu), (set), (exception)),       \
	     (cpu) <= (MAX_HARTS - 1);	                        \
	    )

static unsigned long available_harts[BITMAP_SZ(MAX_HARTS)];
u64 timebase_frequency;

/* Assembly entry point for secondary CPUs */
void secondary_start(void);

/* C entry point for secondary CPUs */
void secondary_cmain(void);

#if defined(MEAS_IPI)
static volatile bool ipi_master;
static spinlock_t ipi_lock;
#endif
static unsigned long boot_hart;

unsigned long eval_ipi_target;

void secondary_cmain(void)
{
	int err;
	pr("Hello world from CPU %lu\n", this_cpu_id());

	irq_disable();
	ext_disable();
	ipi_disable();
	timer_disable();

	/* Unmap bootstrap page tables */
	err = unmap_range(this_root_table_page(),
			  (void *)virt_to_phys(__load_addr), GRINCH_SIZE);
	if (err)
		goto out;

#if defined(MEAS_IPI) /* IPI roundtrip test */
	bool do_ipi = false;

	spin_lock(&ipi_lock);
	if (!ipi_master) {
		ipi_master = true;
		do_ipi = true;
	}
	spin_unlock(&ipi_lock);

	if (do_ipi) {
		pr("HART %lu selected for IPI test. Target HART: %lu\n", this_cpu_id(), boot_hart);

		eval_ipi_target = this_cpu_id();
		irq_disable();

		for (;;) {
			unsigned long c1, c2, t;

			/* Wait some time... */
			t = get_time();
			while(get_time() < t + TIMEBASE_DELTA);

			/* Send IPI to target */
			c1 = get_cycle();
			/* Danger: We don't check errors here */
			sbi_send_ipi((1UL << boot_hart), 0);

			/* Wait until ipi comes back */
			while (!(csr_read(sip) & IE_SIE));
			c2 = get_cycle();

			c2 = c2 - c1;

			print_meas_cyc("IPI RT", c2);
			csr_clear(sip, (1 << IRQ_S_SOFT));
		}
	}
#elif defined(MEAS_RFNC)
	for (;;) {
		unsigned long t;
		struct sbiret ret;

		/* Wait some time... */
		t = get_time();
		while(get_time() < t + TIMEBASE_DELTA);

		ret = sbi_remote_fence_i((1UL << boot_hart), 0);
		if (ret.error) {
			pr("SBI RFENCE error: %ld %ld!\n", ret.error, ret.value);
			goto foo;
		}
	}
foo:
#endif

	pr("Halting CPU %lu\n", this_cpu_id());

out:
	if (err)
		pr("Unable to bring up CPU %lu\n", this_cpu_id());
}

static unsigned int next_cpu(unsigned int cpu, unsigned long *bitmap,
			     unsigned int exception)
{
	do
		cpu++;
	while (cpu <= MAX_HARTS &&
	       (cpu == exception || !test_bit(cpu, bitmap)));
	return cpu;
}

static int boot_cpu(unsigned long hart_id)
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

	pcpu->hartid = hart_id;

	/* Copy over kernel page tables */

	/*
	 * FIXME: This is done by handwork at the moment, and only works with
	 * SV39. This must be improved, if we ever use some other pagers
	 */

	/* Hook in the whole kernel. */
	index = vaddr2vpn(_load_addr, satp_mode == SATP_MODE_39 ? 2 : 3);
	pcpu->root_table_page[index] = this_root_table_page()[index];

	/*
	 * Hook in the percpu stack. Take care, SV48 is not supported, Stack
	 * addresses will overlap with the top-most page table!
	 */
	err = paging_cpu_init(hart_id);
	if (err)
		return err;

	/* The page table must contain a boot trampoline */
	paddr = virt_to_phys(__load_addr);
	map_range(pcpu->root_table_page, (void*)paddr, paddr, GRINCH_SIZE,
		  PAGE_FLAGS_DEFAULT);

	paddr= virt_to_phys(secondary_start);

	/* Make it easy for secondary_entry: provide the content of satp */
	opaque = (virt_to_phys(per_cpu(hart_id)->root_table_page) >> PAGE_SHIFT)
		| satp_mode;

	ret = sbi_hart_start(hart_id, paddr, opaque);
	if (ret.error) {
		pr("Failed to bring up CPU %lu Error: %ld Value: %ld\n",
		   hart_id, ret.error, ret.value);
		return -ENOSYS;
	}

	return 0;
}

int smp_init(void)
{
	unsigned long hart_id;
	int err;

	for_each_cpu_except(hart_id, available_harts, this_cpu_id()) {
		err = boot_cpu(hart_id);
		if (err)
			return err;
	}

	return err;
}

int platform_init(void)
{
	unsigned long hart_id;
	int err, off, child;
	const char *name;
	const fdt32_t *reg;

	off = fdt_path_offset(_fdt, "/cpus");
	if (off < 0) {
		pr("No CPUs found in device-tree. Halting.\n");
		return -ENOENT;
	}

	boot_hart = this_cpu_id();

	fdt_for_each_subnode(child, _fdt, off) {
		name = fdt_get_name(_fdt, child, NULL);
		if (strcmp(name, "cpu-map") == 0)
			continue;

		reg = fdt_getprop(_fdt, child, "reg", &err);
		if (err < 0) {
			pr("%s: Error reading reg\n", name);
			return -EINVAL;
		}
		hart_id = fdt32_to_cpu(reg[0]);
		if (hart_id >= MAX_HARTS) {
			pr("%s: HART %lu beyond MAX_HARTS\n", name, hart_id);
			return -ERANGE;
		}

		if (!fdt_device_is_available(_fdt, child)) {
			pr("%s: HART %lu disabled via device-tree\n", name,
			   hart_id);
			continue;
		}

		bitmap_set(available_harts, hart_id, 1);

		pr("%s: HART %lu available\n", name, hart_id);

		err = page_mark_used(per_cpu(hart_id), PAGES(sizeof(struct per_cpu)));
		if (err)
			return err;
	}

	/* get timebase frequency */
	reg = fdt_getprop(_fdt, off, "timebase-frequency", &err);
	if (err < 0) {
		pr("timebase-frequency not found. Halting.\n");
		return -ENOENT;
	}
	timebase_frequency = fdt32_to_cpu(reg[0]);
	pr("Timebase Frequency: %llu\n", timebase_frequency);

	return 0;
}
