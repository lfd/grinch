/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"bootparam: " x

#include <grinch/bootparam.h>
#include <grinch/fdt.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/symbols.h>

static void parse_param(char *str)
{
	const struct grinch_bootparam *iter;
	char *name, *arg;

	name = strsep(&str, "=");
	arg = strsep(&str, "=");

	for (iter = (const struct grinch_bootparam *)__bootparams_start;
	     iter < (const struct grinch_bootparam *)__bootparams_end;
	     iter++) {
		if (!strcmp(name, iter->name))
			iter->parse(arg);
	}
}

int bootparam_init(void)
{
	char tmp[128], *token, *bootargs_str;
	const char *bootargs;
	int node, len;

	node = fdt_path_offset(_fdt, "/chosen");
	if (node < 0)
		goto empty_out;

	bootargs = fdt_getprop(_fdt, node, "bootargs", &len);
	if (!bootargs)
		goto empty_out;

	pr("Grinch cmdline: %s\n", bootargs);

	strncpy(tmp, bootargs, sizeof(tmp) - 1);
	bootargs_str = tmp;
        while ((token = strsep(&bootargs_str, " "))) {
		if (!strlen(token))
			continue;
		parse_param(token);
	}

	return 0;

empty_out:
	pr("No bootargs provided\n");

	return 0;
}
