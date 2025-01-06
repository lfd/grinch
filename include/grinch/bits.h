/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2025
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* Partly copied from the Linux kernel */

#ifndef _BITS_H
#define _BITS_H

#define BITS_PER_BYTE		8

#define __GENMASK(h, l) \
        (((~_UL(0)) - (_UL(1) << (l)) + 1) & \
         (~_UL(0) >> (__BITS_PER_LONG - 1 - (h))))

#define __GENMASK_ULL(h, l) \
        (((~_ULL(0)) - (_ULL(1) << (l)) + 1) & \
          (~_ULL(0) >> (__BITS_PER_LONG_LONG - 1 - (h))))

#define GENMASK(h, l) __GENMASK(h, l)
#define GENMASK_ULL(h, l) __GENMASK_ULL(h, l)

#endif /* _BITS_H */
