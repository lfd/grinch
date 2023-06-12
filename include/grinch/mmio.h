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

#ifndef _MMIO_H
#define _MMIO_H

#include <grinch/types.h>

#define DEFINE_MMIO_READ(size)                                          \
static inline u##size mmio_read##size(void *address)                    \
{                                                                       \
        return *(volatile u##size *)address;                            \
}

DEFINE_MMIO_READ(8)
DEFINE_MMIO_READ(16)
DEFINE_MMIO_READ(32)
DEFINE_MMIO_READ(64)

#define DEFINE_MMIO_WRITE(size)                                         \
static inline void mmio_write##size(void *address, u##size value)       \
{                                                                       \
        *(volatile u##size *)address = value;                           \
}

DEFINE_MMIO_WRITE(8)
DEFINE_MMIO_WRITE(16)
DEFINE_MMIO_WRITE(32)
DEFINE_MMIO_WRITE(64)

#endif /* _MMIO_H */
