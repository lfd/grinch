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

#ifndef _TYPES_H
#define _TYPES_H

#ifndef __ASSEMBLY__

#define BITS_PER_LONG	64
#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

#define MIN(a, b)		((a) <= (b) ? (a) : (b))
#define __round_mask(x, y)	((__typeof__(x))((y)-1))
#define round_down(x, y)	((x) & ~__round_mask(x, y))

#define NULL	((void*)0)

/* Applies to both, arm64 and riscv64 */
typedef unsigned long long u64;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;

typedef long long s64;
typedef short s16;
typedef int s32;
typedef char s8;

typedef unsigned long size_t;
typedef u64 uintptr_t;
typedef s64 intptr_t;

typedef uintptr_t paddr_t;
typedef intptr_t ptrdiff_t;

typedef unsigned long *pt_entry_t;
typedef pt_entry_t page_table_t;

/* Generic types */
typedef enum { true = 1, false = 0 } bool;


#endif /* __ASSEMBLY__ */

#endif /* _TYPES_H */
