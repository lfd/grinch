/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef _GRINCH_SEMIHOST_H
#define _GRINCH_SEMIHOST_H

#include <grinch/arch/semihost.h>

#define SYS_WRITEC	0x03

static inline void semihosting_putchar(char c)
{
	semihosting_call(SYS_WRITEC, &c);
}

#endif /* _GRINCH_SEMIHOST_H */
