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

#include <grinch/string.h>
#include <grinch/alloc.h>

#define STRDUP		kstrdup
#define STRNDUP		kstrndup
#define ALLOCATOR	kmalloc

#include "../common/src/string.c"
