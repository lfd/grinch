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

#ifndef __COMPILER_ATTRIBUTES_H
#define __COMPILER_ATTRIBUTES_H

#define __printf(a, b)		__attribute__((__format__(printf, a, b)))
#define __noreturn		__attribute__((noreturn))

#define __always_inline		inline __attribute__((always_inline))
#define noinline		__attribute__((__noinline__))

#define __used			__attribute__((__used__))

#define __aligned(x)		__attribute__((__aligned__(x)))

#define __packed		__attribute__((__packed__))

#define __section(section)	__attribute__((__section__(section)))

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#if __has_attribute(__fallthrough__)
# define fallthrough                    __attribute__((__fallthrough__))
#else
# define fallthrough                    do {} while (0)  /* fallthrough */
#endif

#endif /* __COMPILER_ATTRIBUTES_H */
