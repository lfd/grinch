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

#include <reboot.h>
#include <syscall.h>
#include <grinch/reboot_abi.h>

int reboot(unsigned int cmd)
{
	return syscall(SYS_reboot, GRINCH_REBOOT_MAGIC, cmd);
}
