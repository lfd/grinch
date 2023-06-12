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

#include <grinch/compiler_attributes.h>
#include <grinch/string.h>

int memcmp(const void *dst, const void *src, size_t count)
{
	register int r;
	register const unsigned char *d, *s;

	d = dst;
	s = src;
	++count;

	while (likely(--count)) {
		if (unlikely(r=(*d - *s)))
			return r;
		++d;
		++s;
	}

	return 0;
}

void *memset(void *s, int c, size_t n)
{
	u8 *p = s;

	while (n-- > 0)
		*p++ = c;
	return s;
}

void *memmove(void *dst, const void *src, size_t count)
{
	char *a = dst;
	const char *b = src;
	if (src!=dst) {
		if (src > dst)
			while (count--) *a++ = *b++;
		else {
			a += count - 1;
			b += count - 1;
			while (count--) *a-- = *b--;
		}
	}
	return dst;
}

void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *pc;

	pc = (unsigned char *)s;
	for (; n--; pc++)
		if (*pc == c)
			return ((void *) pc);
	return 0;
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2) {
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strrchr(const char *t, int c) {
	register char ch;
	register const char *l=0;

	ch = c;
	for (;;) {
		if (unlikely(*t == ch))
			l = t;
		if (unlikely(!*t))
			return (char*)l;
		++t;
	}

	return (char*)l;
}

size_t strlen(const char *s)
{
	register size_t i;

	if (unlikely(!s))
		return 0;
	for (i=0; likely(*s); ++s)
		++i;

	return i;
}


void *memcpy(void *dest, const void *src, size_t n)
{
	const u8 *s = src;
	u8 *d = dest;

	while (n-- > 0)
		*d++ = *s++;
	return dest;
}
