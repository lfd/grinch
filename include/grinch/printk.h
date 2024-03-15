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

#ifndef _PRINTK_H
#define _PRINTK_H

#include <grinch/compiler_attributes.h>
#include <grinch/init.h>

void _puts(const char *msg); /* No prefix */
void __printf(1, 2) printk(const char *fmt, ...);

void printk_init(void);

#define	PR_SOH			"\001"
#define PR_WARN			PR_SOH "0"
#define PR_INFO			PR_SOH "1"
#define PR_DEBUG		PR_SOH "9"
#define	PR_NOPREFIX		PR_SOH "x"

#define PR_DEFAULT		PR_INFO

#define PR_SOH_ASCII		(PR_SOH[0])
#define PR_NOPREFIX_ASCII	(PR_NOPREFIX[1])

#ifndef dbg_fmt
#define dbg_fmt(__x)	__x
#endif /* dbg_fmt */

/* Logging accessors/helpers */
#define _prr(header, fmt, ...)	printk(header fmt, ##__VA_ARGS__)
#define _prri(header, fmt, ...)	printk(ISTR(header fmt), ##__VA_ARGS__)

#define _pr(header, fmt, ...)	_prr(header, dbg_fmt(fmt), ##__VA_ARGS__)
#define _pri(header, fmt, ...)	_prri(header, dbg_fmt(fmt), ##__VA_ARGS__)

/* For default loglevel */
#define pr(fmt, ...)		_pr(PR_DEFAULT, fmt, ##__VA_ARGS__)
#define pri(fmt, ...)		_pri(PR_DEFAULT, fmt, ##__VA_ARGS__)

/* Other loglevels */
#define pr_warn(fmt, ...)	_pr(PR_WARN, fmt, ##__VA_ARGS__)
#define pr_warn_i(fmt, ...)	_pri(PR_WARN, fmt, ##__VA_ARGS__)

#define pr_info(fmt, ...)	_pr(PR_INFO, fmt, ##__VA_ARGS__)
#define pr_info_i(fmt, ...)	_pri(PR_INFO, fmt, ##__VA_ARGS__)

#define pr_dbg(fmt, ...)	_pr(PR_DEBUG, fmt, ##__VA_ARGS__)
#define pr_dbg_i(fmt, ...)	_pri(PR_DEBUG, fmt, ##__VA_ARGS__)

/* special stuff */
#define pr_raw(fmt, ...)	_prr(PR_DEFAULT PR_NOPREFIX, fmt, ##__VA_ARGS__)
#define pr_raw_i(fmt, ...)	_prri(PR_DEFAULT PR_NOPREFIX, fmt, ##__VA_ARGS__)

#define pr_raw_dbg(fmt, ...)	_prr(PR_DEBUG PR_NOPREFIX, fmt, ##__VA_ARGS__)
#define pr_raw_dbg_i(fmt, ...)	_prri(PR_DEBUG PR_NOPREFIX, fmt, ##__VA_ARGS__)

#define prg(fmt, ...) do { if (grinch_is_guest) { pr(fmt, ##__VA_ARGS__); } } while (false)
#define prh(fmt, ...) do { if (!grinch_is_guest) { pr(fmt, ##__VA_ARGS__); } } while (false)


#define trace_error(code) ({						\
	pr_dbg("%s:%d: returning error %pe\n",				\
	       __FILE__, __LINE__, ERR_PTR(code));			\
	code;								\
})

#endif /* _PRINTK_H */
