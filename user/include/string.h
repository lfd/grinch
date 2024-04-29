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

/* include the common header */
#include <grinch/string.h>

char *strdup(const char *s);

/* counts the number of occurences of c in s */
unsigned int strcount(const char *s, char c);

#endif /* _STRING_H */
