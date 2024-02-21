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

#ifndef _VSPRINTF_H
#define _VSPRINTF_H

#include <stdarg.h>
#include <stddef.h>
#include <grinch/compiler_attributes.h>

int __printf(3, 0)
vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

int __printf(3, 4) snprintf(char *buf, size_t size, const char *fmt, ...);

#endif /* _VSPRINTF_H */
