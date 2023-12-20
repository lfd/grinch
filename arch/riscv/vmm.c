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

#define dbg_fmt(x)	"vmm: " x

#include <grinch/errno.h>
#include <grinch/printk.h>
#include <grinch/vmm.h>

int vmm_init(void)
{
	return -ENOSYS;
}
