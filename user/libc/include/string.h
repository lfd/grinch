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

#ifndef _STRING_H
#define _STRING_H

#include <grinch/string_common.h>

const char *strerror(int err);

char *strdup(const char *s);
char *strndup(const char *s, size_t n);

#endif /* _STRING_H */
