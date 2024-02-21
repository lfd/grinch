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

/* Copied from the Linux kernel */

#ifndef _CTYPE_H
#define _CTYPE_H

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */

extern const unsigned char _ctype[];

#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

#define isalnum(c)	((__ismask(c)&(_U|_L|_D)) != 0)
#define isxdigit(c)	((__ismask(c)&(_D|_X)) != 0)
#define isprint(c)	((__ismask(c)&(_P|_U|_L|_D|_SP)) != 0)

#if __has_builtin(__builtin_isdigit)
#define isdigit(c)	__builtin_isdigit(c)
#else
static inline int isdigit(int c)
{
	return '0' <= c && c <= '9';
}
#endif

static inline char _tolower(const char c)
{
	return c | 0x20;
}

#endif /* _CTYPE_H */
