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

#ifndef _BOOTPARAM_H
#define _BOOTPARAM_H

#include <grinch/init.h>
#include <grinch/types.h>
#include <grinch/strtox.h>

struct grinch_bootparam {
	const char *name;
	void (*parse)(const char *);
};

#define bootparam(NAME, PARSE)						\
static const char __bootparam_##NAME##_str[] __initconst = #NAME;	\
									\
static const struct grinch_bootparam __bootparam_##NAME			\
	__initbootparams = {						\
	.name = __bootparam_##NAME##_str,				\
	.parse = PARSE,							\
}

int bootparam_init(void);
int bootparam_parse_size(const char *str, size_t *res);

#endif /* _BOOTPARAM_H */
