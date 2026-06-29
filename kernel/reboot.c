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

#include <grinch/arch.h>
#include <grinch/errno.h>
#include <grinch/syscall.h>
#include <grinch/reboot_abi.h>

SYSCALL_DEF2(reboot, int, magic, unsigned int, cmd)
{
	if (magic != GRINCH_REBOOT_MAGIC)
		return -EINVAL;

	switch (cmd) {
	case GRINCH_REBOOT_CMD_HALT:
		arch_shutdown(0);
	case GRINCH_REBOOT_CMD_REBOOT:
		arch_reboot();
	default:
		return -EINVAL;
	}
}
