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

#define BYTES_PER_LONG		(__SIZEOF_POINTER__)
#define BITS_PER_LONG		(BYTES_PER_LONG * 8)
#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

#define MIN(a, b)		((a) <= (b) ? (a) : (b))
#define __round_mask(x, y)	((__typeof__(x))((y)-1))
#define round_down(x, y)	((x) & ~__round_mask(x, y))

/* Applies to all, arm64, riscv32 and riscv64 */
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef long long s64;
typedef int s32;
typedef short s16;
typedef char s8;

typedef unsigned long long __u64;
typedef unsigned int __u32;
typedef unsigned short __u16;
typedef unsigned char __u8;

typedef long long __s64;
typedef int __s32;
typedef short __s16;
typedef char __s8;

typedef u64 uint64_t;
typedef u32 uint32_t;
typedef u16 uint16_t;
typedef u8 uint8_t;

typedef s64 int64_t;
typedef s32 int32_t;
typedef s16 int16_t;
typedef s8 int8_t;

typedef unsigned long uintptr_t;
typedef signed long intptr_t;

typedef uintptr_t paddr_t;
typedef intptr_t ptrdiff_t;

typedef unsigned long *pt_entry_t;
typedef pt_entry_t page_table_t;

/* Generic types */
#include <grinch/stdbool.h>

typedef int pid_t;
typedef long long loff_t;
typedef unsigned short mode_t;

#define __mayuser
#define __user

#include <stddef.h>

#endif /* __ASSEMBLY__ */

#endif /* _TYPES_H */
