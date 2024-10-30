/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _GRINCH_STRING_H
#define _GRINCH_STRING_H

#include <grinch/types.h>

#ifndef __ASSEMBLY__

void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *dst, const void *src, size_t count);
void *memset(void *s, int c, size_t n);
void *memchr(const void *s, int c, size_t n);
int memcmp(const void *dst, const void *src, size_t count);

char *strchr(const char *s, int c);
char *strpbrk(const char *cs, const char *ct);
char *strncpy(char *dest, const char *source, size_t count);
char *strsep(char **s, const char *ct);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t count);

size_t strnlen(const char *s,size_t maxlen);
size_t strlen(const char *s);
char *strrchr(const char *t, int c);
char *strchrnul(const char *s, int c);

/* counts the number of occurences of c in s */
unsigned int strcount(const char *s, char c);

#endif /* __ASSEMBLY__ */

#endif /* _GRINCH_STRING_H */
