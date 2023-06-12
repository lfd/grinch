/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _STRING_H
#define _STRING_H

#include <grinch/types.h>

#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)

#ifndef __ASSEMBLY__

void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *dst, const void *src, size_t count);
void *memset(void *s, int c, size_t n);
void *memchr(const void *s, int c, size_t n);
int memcmp(const void *dst, const void *src, size_t count);
int strcmp(const char *s1, const char *s2);

size_t strnlen(const char *s,size_t maxlen);
size_t strlen(const char *s);
char *strrchr(const char *t, int c);

#endif /* __ASSEMBLY__ */

#endif /* _STRING_H */
