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

#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

#include <grinch/string.h>
#include <grinch/swab.h>
#include <grinch/types.h>

#define fdt32_to_cpu(x) __be32_to_cpu(x)
#define cpu_to_fdt32(x) __cpu_to_be32(x)
#define fdt64_to_cpu(x) __be64_to_cpu(x)
#define cpu_to_fdt64(x) __cpu_to_be64(x)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __be32_to_cpu(x) __swab32((u32)(x))
#define __cpu_to_be32(x) __swab32((u32)(x))

#define __be64_to_cpu(x) __swab64((u64)(x))
#define __cpu_to_be64(x) __swab64((u64)(x))
#else
#error "NOT IMPLEMENTED"
#endif

typedef uint16_t fdt16_t;
typedef uint32_t fdt32_t;
typedef uint64_t fdt64_t;

#endif /* _LIBFDT_ENV_H */
