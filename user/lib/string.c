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

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define STRDUP		strdup
#define STRNDUP		strndup
#define ALLOCATOR	malloc

#include "../../common/src/string.c"

const char *strerror(int err)
{
	return errname(err);
}
