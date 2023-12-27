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

#ifndef _BOOTPARAM_H
#define _BOOTPARAM_H

#define __init	__attribute__((section(".init.text"), used))

struct grinch_bootparam {
	const char *name;
	void (*parse)(const char *);
};

#define bootparam(NAME, PARSE)						\
static const char bootparam_##NAME##_str[]				\
	__attribute__((section(".init.rodata"), used)) = #NAME;		\
									\
static const struct grinch_bootparam bootparam_##NAME 			\
	__attribute__((section(".init.bootparams"), used)) = {		\
	.name = bootparam_##NAME##_str,					\
	.parse = PARSE,							\
}

int bootparam_init(void);

#endif /* _BOOTPARAM_H */
