/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"reboot: " x

#include <grinch/errno.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/reboot.h>
#include <grinch/syscall.h>
#include <grinch/reboot_abi.h>

int (*arch_shutdown)(int err);
int (*arch_reboot)(void);

void __noreturn shutdown(int err)
{
	pr("Shutdown. Reason: %pe\n", ERR_PTR(err));

	if (arch_shutdown)
		err = arch_shutdown(err);
	else
		panic("No shutdown method\n");

	panic("Shutdown failed: %pe\n", ERR_PTR(err));
}

void __noreturn reboot(void)
{
	int err;

	if (arch_reboot)
		err = arch_reboot();
	else
		panic("No reboot method\n");

	panic("Reboot failed: %pe\n", ERR_PTR(err));
}

SYSCALL_DEF2(reboot, int, magic, unsigned int, cmd)
{
	if (magic != GRINCH_REBOOT_MAGIC)
		return -EINVAL;

	switch (cmd) {
	case GRINCH_REBOOT_CMD_HALT:
		shutdown(0);
	case GRINCH_REBOOT_CMD_REBOOT:
		reboot();
	default:
		return -EINVAL;
	}
}
