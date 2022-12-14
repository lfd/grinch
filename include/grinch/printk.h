/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _PRINTK_H
#define _PRINTK_H

void puts(const char *msg);
void _puts(const char *msg);
void __attribute__((format(printf, 1, 2))) printk(const char *fmt, ...);

#ifndef dbg_fmt
#define dbg_fmt(__x) (__x)
#endif /* dbg_fmt */

#define pr(fmt, ...) printk(dbg_fmt(fmt), ##__VA_ARGS__)
#define ps(str) puts(dbg_fmt(str))

#define trace_error(code) ({						  \
	printk("%s:%d: returning error %s\n", __FILE__, __LINE__, #code); \
	code;								  \
})

#ifdef DEBUG
#define pd(fmt, ...) printk(dbg_fmt(fmt), ##__VA_ARGS__)
#else /* !DEBUG */
#define pd(...) do{ } while ( false )
#endif /* DEBUG */

#endif /* _PRINTK_H */
