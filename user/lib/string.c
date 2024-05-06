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

#include <string.h>
#include <stdlib.h>

#include "../../common/src/string.c"

char *strdup(const char *s)
{
	size_t len;
	char *tmp;

	if (!s)
		return NULL;

	len = strlen(s) + 1;
	tmp = malloc(len);
	if (tmp)
		memcpy(tmp, s, len);

	return tmp;
}
