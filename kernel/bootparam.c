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
#include <grinch/errno.h>
#include <grinch/const.h>
#include <grinch/fdt.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/symbols.h>

int bootparam_parse_size(const char *str, size_t *sz)
{
	unsigned long int ret;
	size_t len;
	char *ep;

	ret = strtoul(str, &ep, 10);
	if (!ret)
		return -EINVAL;

	len = strlen(ep);
	if (len > 1)
		return -EINVAL;

	if (len == 0)
		goto out;

	switch (ep[0]) {
		case 'K':
			ret *= KIB;
			break;

		case 'M':
			ret *= MIB;
			break;

		case 'G':
			ret *= GIB;
			break;

		default:
			return -EINVAL;
	}

out:
	*sz = ret;
	return 0;
}

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
