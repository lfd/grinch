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

#ifndef _LIMITS_H
#define _LIMITS_H

#define INT_MAX		((int)(~0U>>1))
#define UINT32_MAX	((u32)~0U)
#define INT32_MAX	((s32)(UINT32_MAX >> 1))
#define ULLONG_MAX	(~0ULL)
#define ULONG_MAX	(~0UL)

#define PATH_MAX	4096

#endif /* _LIMITS_H */
