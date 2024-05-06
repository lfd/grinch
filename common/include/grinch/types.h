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

#ifndef _TYPES_H
#define _TYPES_H

#ifndef __ASSEMBLY__

#define BITS_PER_LONG	64
#define BYTES_PER_LONG	(BITS_PER_LONG / 8)
#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

#define INT_MAX		((int)(~0U>>1))
#define UINT32_MAX	((u32)~0U)
#define INT32_MAX	((s32)(UINT32_MAX >> 1))
#define ULLONG_MAX	(~0ULL)
#define ULONG_MAX	(~0UL)

#define MIN(a, b)		((a) <= (b) ? (a) : (b))
#define __round_mask(x, y)	((__typeof__(x))((y)-1))
#define round_down(x, y)	((x) & ~__round_mask(x, y))

/* Applies to both, arm64 and riscv64 */
typedef unsigned long long u64;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;

typedef long long s64;
typedef short s16;
typedef int s32;
typedef char s8;

typedef unsigned long long __u64;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef unsigned char __u8;

typedef long long __s64;
typedef short __s16;
typedef int __s32;
typedef char __s8;

typedef u64 uintptr_t;
typedef s64 intptr_t;

typedef uintptr_t paddr_t;
typedef intptr_t ptrdiff_t;

typedef unsigned long *pt_entry_t;
typedef pt_entry_t page_table_t;

/* Generic types */
typedef enum { true = 1, false = 0 } bool;

typedef int pid_t;
typedef long long loff_t;
typedef unsigned short mode_t;

#define __mayuser
#define __user

#include <stddef.h>

#endif /* __ASSEMBLY__ */

#endif /* _TYPES_H */
