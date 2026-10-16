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

#ifndef _ARCH_CONSOLE_H
#define _ARCH_CONSOLE_H

#ifdef CONFIG_SBI_TTY
#define DEFAULT_CONSOLE		"ttySBI"
#else
#define DEFAULT_CONSOLE		"null"
#endif

#ifdef CONFIG_EARLYCON_SEMIHOST
#include <grinch/semihost.h>
#define arch_early_putc(c)	semihosting_putchar(c)
#elif defined(CONFIG_EARLYCON_SBI)
#include <grinch/arch/sbi.h>
#define arch_early_putc(c)	sbi_console_putchar(c)
#elif defined(CONFIG_EARLYCON_MMIO)
#include <grinch/mmio.h>
#define arch_early_putc(c)	mmio_write32((void *)CONFIG_EARLYCON_MMIO_BASE, \
					     (unsigned char)(c))
#endif

#ifdef arch_early_putc
/* As uart_write_char does for the console that takes over later. */
static void arch_early_dbg(const char *str, unsigned int len)
{
	while (len--) {
		if (*str == '\n')
			arch_early_putc('\r');
		arch_early_putc(*str++);
	}
}
#else
#define arch_early_dbg		NULL
#endif

#endif /* _ARCH_CONSOLE_H */
