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

#ifndef _PRINTK_H
#define _PRINTK_H

#include <grinch/compiler_attributes.h>
#include <grinch/init.h>

void puts(const char *msg); /* Prefixed */
void _puts(const char *msg); /* No prefix */
void __printf(1, 2) printk(const char *fmt, ...);
void __printf(1, 2) panic(const char *fmt, ...) __noreturn;

void console_flush(void);
void printk_init(void);

#ifndef dbg_fmt
#define dbg_fmt(__x) (__x)
#endif /* dbg_fmt */

#define pr(fmt, ...) printk(dbg_fmt(fmt), ##__VA_ARGS__)
#define ps(str) puts(dbg_fmt(str))

#define prg(fmt, ...) if (grinch_is_guest) { pr(fmt, ##__VA_ARGS__); }

#define pri(fmt, ...) printk(ISTR(dbg_fmt(fmt)), ##__VA_ARGS__)
#define psi(str) puts(ISTR(dbg_fmt(str)))

#define trace_error(code) ({						  \
	printk("%s:%d: returning error %d\n", __FILE__, __LINE__, code); \
	code;								  \
})

#ifdef DEBUG
#define pd(fmt, ...) printk(dbg_fmt(fmt), ##__VA_ARGS__)
#define pdi(fmt, ...) printk(ISTR(dbg_fmt(fmt)), ##__VA_ARGS__)
#else /* !DEBUG */
#define pd(...) do{ } while ( false )
#define pdi(...) do{ } while ( false )
#endif /* DEBUG */

#endif /* _PRINTK_H */
