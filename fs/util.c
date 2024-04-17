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

#include <grinch/fs/util.h>

bool pathname_sanitise_dir(char *pathname)
{
	size_t len;

	len = strlen(pathname);
	if (len && pathname[len - 1] == '/') {
		pathname[len - 1] = 0;
		return true;
	}

	return false;
}
