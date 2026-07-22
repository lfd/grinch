/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"platform: " x

#include <asm/sysregs.h>

#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/init.h>
#include <grinch/percpu.h>
#include <grinch/platform.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/string.h>

int __init arch_platform_init(void)
{
	unsigned long cpuid, mpidr;
	struct per_cpu *pcpu;
	int err, off, child;
	const fdt32_t *reg;
	const char *name;

	off = fdt_path_offset(_fdt, "/cpus");
	if (off < 0) {
		pri("No CPUs found in device-tree. Halting.\n");
		return -ENOSYS;
	}

	fdt_for_each_subnode(child, _fdt, off) {
		name = fdt_get_name(_fdt, child, NULL);
		if (strcmp(name, ISTR("cpu-map")) == 0)
			continue;

		reg = fdt_getprop(_fdt, child, ISTR("reg"), &err);
		if (err < 0) {
			pri("%s: Error reading reg\n", name);
			return -EINVAL;
		}
		mpidr = fdt32_to_cpu(reg[0]) & MPIDR_CPUID_MASK;

		/*
		 * We use the lowest affinity level as the dense logical CPU
		 * id, matching the slot selection in head.S.
		 */
		cpuid = MPIDR_AFFINITY_LEVEL(mpidr, 0);
		if (cpuid >= MAX_CPUS) {
			pri("%s: CPU %lu beyond MAX_CPUS\n", name, cpuid);
			return -ERANGE;
		}

		if (!fdt_device_is_available(_fdt, child)) {
			pri("%s: CPU %lu disabled via device-tree\n", name,
			    cpuid);
			continue;
		}

		bitmap_set(cpus_available, cpuid, 1);
		pri("%s: CPU %lu (MPIDR 0x%lx) available\n", name, cpuid,
		    mpidr);

		pcpu = per_cpu(cpuid);
		if (cpuid != this_cpu_id())
			memset(pcpu, 0, sizeof(*pcpu));
		pcpu->mpidr = mpidr;
		pcpu->of_node = child;
	}

	/* Free per_cpu pages of absent CPUs back to the pool */
	for (cpuid = 0; cpuid < MAX_CPUS; cpuid++)
		if (!test_bit(cpuid, cpus_available))
			free_pages(per_cpu(cpuid),
				   PAGES(sizeof(struct per_cpu)));

	return 0;
}
