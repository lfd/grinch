/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
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

#define stdout	1

int puts(const char *s);
void __printf(1, 2) printf(const char *fmt, ...);

#endif /* _STDIO_H */
