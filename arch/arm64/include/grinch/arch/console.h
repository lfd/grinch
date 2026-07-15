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

#ifdef CONFIG_ARM64_SEMIHOSTING
static inline void arch_early_dbg_c(char c)
{
    register unsigned long x0 asm("x0") = 0x03; // SYS_WRITEC
    register const char *x1 asm("x1") = &c;

    asm volatile(
        "hlt #0xf000"
        :
        : "r"(x0), "r"(x1)
        : "memory");
}

static void arch_early_dbg(const char *str, unsigned int len)
{
    while (len--)
	    arch_early_dbg_c(*str++);
}
#else
#define arch_early_dbg		NULL
#endif

#endif /* _ARCH_CONSOLE_H */
