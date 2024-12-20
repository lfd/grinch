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

#include <errno.h>
#include <sys/auxv.h>

#include <_internal.h>

unsigned long getauxval(unsigned long item)
{
	size_t *auxv;

	for (auxv = __libc.auxv; *auxv; auxv += 2)
		if (*auxv == item)
			return auxv[1];

	errno = ENOENT;
	return 0;
}
