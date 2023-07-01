/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <unistd.h>
#include <syscall.h>

ssize_t write(int fd, const void *buf, size_t count)
{
	unsigned long ret;

	ret = syscall(SYS_write, (unsigned long)fd, (unsigned long)buf,
		      (unsigned long)count, 0, 0, 0);

	return (ssize_t)ret;
}
