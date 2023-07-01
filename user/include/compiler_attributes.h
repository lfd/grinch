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

#ifndef __COMPILER_ATTRIBUTES_H
#define __COMPILER_ATTRIBUTES_H

#define __printf(a, b)	__attribute__((__format__(printf, a, b)))
#define __noreturn	__attribute__((noreturn))
#define __always_inline	inline __attribute__((always_inline))

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#endif /* __COMPILER_ATTRIBUTES_H */
