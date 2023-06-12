/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/errno.h>
#include <grinch/irq.h>
#include <grinch/printk.h>
#include <grinch/sbi.h>
#include <grinch/sbi_handler.h>

int handle_ipi(void)
{
	return -ENOSYS;
}

int handle_timer(void)
{
	printk("Timer fired!\n");
	//sbi_set_timer(-1);
	sbi_set_timer(get_time() + 0x400000);
	return 0;
}

int handle_ecall(struct registers *regs)
{
	return -ENOSYS;
}
