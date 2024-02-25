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
#include <fcntl.h>
#include <syscall.h>

int open(const char *pathname, int flags)
{
	int ret;

	ret = syscall_2(SYS_open,
			(unsigned long)pathname, (unsigned long)flags);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
