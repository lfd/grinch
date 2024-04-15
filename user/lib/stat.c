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

#include <sys/stat.h>

#include <errno.h>
#include <syscall.h>

int stat(const char *pathname, struct stat *statbuf)
{
	int ret;

	ret = syscall_2(SYS_stat, (unsigned long)pathname,
			(unsigned long)statbuf);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
