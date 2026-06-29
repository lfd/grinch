/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _GRINCH_REBOOT_ABI_H
#define _GRINCH_REBOOT_ABI_H

#define GRINCH_REBOOT_MAGIC	((int)0xb007b007)

#define GRINCH_REBOOT_CMD_HALT		0x0
#define GRINCH_REBOOT_CMD_REBOOT	0x1

#endif /* _GRINCH_REBOOT_ABI_H */
