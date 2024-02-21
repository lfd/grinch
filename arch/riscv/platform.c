/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"platform: " x

#include <asm/isa.h>

#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/printk.h>
#include <grinch/platform.h>
#include <grinch/string.h>

const char *platform_model = "PLATFORM,UNKNOWN";

int __init platform_init(void)
{
	const char *name, *isa;
	unsigned long hart_id;
	int err, off, child;
	const fdt32_t *reg;

	bitmap_set(cpus_online, this_cpu_id(), 1);

	off = fdt_path_offset(_fdt, "/");
	if (off < 0)
		goto no_model;

	name = fdt_getprop(_fdt, off, ISTR("model"), &err);
	if (name)
		platform_model = name;
no_model:
	pri("Found platform: %s\n", platform_model);

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
		hart_id = fdt32_to_cpu(reg[0]);
		if (hart_id >= MAX_CPUS) {
			pri("%s: HART %lu beyond MAX_CPUS\n", name, hart_id);
			return -ERANGE;
		}

		isa = fdt_getprop(_fdt, child, ISTR("riscv,isa"), &err);
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
