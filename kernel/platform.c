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

#include <grinch/fdt.h>
#include <grinch/platform.h>
#include <grinch/printk.h>
#include <grinch/smp.h>

const char *platform_model = "PLATFORM,UNKNOWN";

int __init platform_init(void)
{
	const char *name;
	int err, off;

	bitmap_set(cpus_online, this_cpu_id(), 1);

	off = fdt_path_offset(_fdt, "/");
	if (off < 0)
		goto no_model;

	name = fdt_getprop(_fdt, off, ISTR("model"), &err);
	if (name)
		platform_model = name;
no_model:
	pri("Found platform: %s\n", platform_model);

	return arch_platform_init();
}
