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
#include <syscall.h>

int getdents(int fd, struct grinch_dirent *dents, unsigned int size)
{
	int ret;

	ret = syscall_3(SYS_getdents, fd, (unsigned long)dents, size);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
