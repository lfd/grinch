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

#include <dirent.h>

#include <errno.h>
#include <limits.h>
#include <syscall.h>

int getdents(int fd, struct dirent *dents, size_t len)
{
	if (len > INT_MAX)
		len = INT_MAX;

	return syscall(SYS_getdents, fd, dents, len);
}
