/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _STDIO_H
#define _STDIO_H

#include <grinch/compiler_attributes.h>
#include <grinch/types.h>

#define stdin	0
#define stdout	1
#define stderr	2

#define EOF	(-1)

#define APP_NAME(x)	const char __app_name_fmt[] = "[" #x " %u] "

extern const char __app_name_fmt[];

int puts(const char *s);
int __printf(2, 3) dprintf(int fd, const char *fmt, ...);
int __printf(1, 2) printf(const char *fmt, ...);

void printf_set_prefix(bool on);

void perror(const char *s);

#endif /* _STDIO_H */
