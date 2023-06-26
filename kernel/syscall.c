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

#define dbg_fmt(x)	"task: " x

#include <grinch/errno.h>
#include <grinch/syscall.h>
#include <grinch/printk.h>

static void putc(char c)
{
	char tmp[2] = {c, 0};

	puts(tmp);
}

int syscall(unsigned long no, unsigned long arg1,
	    unsigned long arg2, unsigned long arg3,
	    unsigned long arg4, unsigned long arg5,
	    unsigned long arg6, unsigned long *ret)
{
	if (no == 0) {
		putc(arg1);
		*ret = 0;
		return 0;
	}

	*ret = -ENOSYS;
	return -ENOSYS;
}
