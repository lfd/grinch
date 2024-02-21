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

#ifndef _STDDEF_H
#define _STDDEF_H

#define NULL	((void *) 0)
#define offsetof(TYPE, MEMBER)	__builtin_offsetof(TYPE, MEMBER)

typedef unsigned long size_t;
typedef signed long ssize_t;

#endif /* _STDDEF_H */
