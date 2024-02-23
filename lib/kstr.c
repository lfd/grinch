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

#include <grinch/alloc.h>
#include <grinch/kstr.h>

char *kstrdup(const char *s)
{
	size_t len;
	char *tmp;

	if (!s)
		return NULL;

	len = strlen(s) + 1;
	tmp = kmalloc(len);
	if (tmp)
		memcpy(tmp, s, len);

	return tmp;
}
