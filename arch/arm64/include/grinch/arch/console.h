/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ARCH_CONSOLE_H
#define _ARCH_CONSOLE_H

#define DEFAULT_CONSOLE		"null"

#ifdef CONFIG_EARLYCON_SEMIHOST
#include <grinch/semihost.h>
static void arch_early_dbg(const char *str, unsigned int len)
{
	while (len--)
		semihosting_putchar(*str++);
}
#else
#define arch_early_dbg		NULL
#endif

#endif /* _ARCH_CONSOLE_H */
