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

#include <grinch/gfp.h>
#include <grinch/irqchip.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/symbols.h>
#include <grinch/task.h>

DECLARE_BITMAP(cpus_available, MAX_CPUS);
DECLARE_BITMAP(cpus_online, MAX_CPUS);

/* Identity-mapped trampoline root the secondaries boot on. */
page_table_t secondary_boot_root;

unsigned int next_cpu(unsigned int cpu, unsigned long *bitmap,
		      unsigned int exception)
{
	do
		cpu++;
	while (cpu <= MAX_CPUS &&
	       (cpu == exception || !test_bit(cpu, bitmap)));
	return cpu;
}

int __init smp_init(void)
{
	unsigned long cpu, cpus;
	paddr_t paddr;
	int err;

	secondary_boot_root = zalloc_pages(1);
	if (!secondary_boot_root)
		return -ENOMEM;

	/* Let the arch populate its view of the root before we map it. */
	arch_smp_bringup_init();

	/* Identity-map grinch so secondaries run at their physical PC. */
	paddr = v2p(grinch_base());
	err = map_range(secondary_boot_root, (void *)paddr, paddr,
			GRINCH_SIZE, GRINCH_MEM_RX);
	if (err)
		goto out_free;

	cpus = 1;
	for_each_available_cpu_except_this(cpu) {
		err = arch_boot_cpu(cpu);
		if (err) {
			pri("Unable to boot CPU %lu\n", cpu);
			continue;
		}

		while (!test_bit(cpu, cpus_online))
			cpu_relax();
		pri("CPU %lu online!\n", cpu);
		cpus++;
	}

	pri("Successfully brought up %lu CPUs\n", cpus);

	/* The trampoline is only needed during bring-up. */
	unmap_range(secondary_boot_root, (void *)paddr, GRINCH_SIZE);
	err = 0;

out_free:
	free_pages(secondary_boot_root, 1);
	secondary_boot_root = NULL;

	return err;
}

/* Called from the arch boot asm; no C caller, so declared here only. */
void secondary_cmain(void);

void secondary_cmain(void)
{
	arch_secondary_init();
	irqchip_cpu_init();

	cpu_set_online(this_cpu_id());
	mb();

	/*
	 * Enter the scheduler: run a runnable task, or idle until this CPU is
	 * assigned one.
	 */
	prepare_user_return();
}

void ipi_broadcast(void)
{
	unsigned long cpu;

	for_each_online_cpu_except_this(cpu)
		ipi_send(cpu);
}

void check_events(void)
{
	struct per_cpu *tpcpu;

	tpcpu = this_per_cpu();
	spin_lock(&tpcpu->remote_call.lock);
	if (tpcpu->remote_call.active) {
		tpcpu->remote_call.func(tpcpu->remote_call.info);
		tpcpu->remote_call.active = false;
		mb();
	}
	spin_unlock(&tpcpu->remote_call.lock);
}

void on_each_cpu(smp_call_func_t func, void *info)
{
	unsigned long cpu;
	struct per_cpu *pcpu;

	/* local execution */
	func(info);

	/* remote execution */
	for_each_online_cpu_except_this(cpu) {
		pcpu = per_cpu(cpu);
retry:
		spin_lock(&pcpu->remote_call.lock);
		if (pcpu->remote_call.active) {
			spin_unlock(&pcpu->remote_call.lock);
			cpu_relax();
			goto retry;
		}

		pcpu->remote_call.func = func;
		pcpu->remote_call.info = info;
		pcpu->remote_call.active = true;
		spin_unlock(&pcpu->remote_call.lock);
	}

	ipi_broadcast();

	/* wait for completion */
	for_each_online_cpu_except_this(cpu) {
		pcpu = per_cpu(cpu);
		while (pcpu->remote_call.active)
			cpu_relax();
	}
}
