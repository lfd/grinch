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

#include <grinch/compiler_attributes.h>
#include <grinch/percpu.h>

#define PANIC_PREFIX	"P A N I C: "

extern bool is_panic;

void check_panic(void);
void __noreturn panic_stop(void);
void __noreturn do_panic(void);

void __printf(1, 2) panic(const char *fmt, ...) __noreturn;

#define BUG()						\
do {							\
	panic("BUG found at %s:%d on CPU %lu :-(\n",	\
	       __FILE__, __LINE__, this_cpu_id());	\
} while (0)

#endif /* _PANIC_H */
