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

#ifndef _PANIC_H
#define _PANIC_H

#include <grinch/percpu.h>
#include <grinch/printk.h>

void check_panic(void);

void __printf(1, 2) _panic(const char *fmt, ...) __noreturn;

#define panic(fmt, ...)	_panic(PR_CRIT fmt, ##__VA_ARGS__)

#define BUG()						\
do {							\
	panic("BUG found at %s:%d on CPU %lu :-(\n",	\
	       __FILE__, __LINE__, this_cpu_id());	\
} while (0)

#endif /* _PANIC_H */
