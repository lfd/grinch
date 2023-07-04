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

pid_t fork(void)
{
	pid_t ret;

	ret = syscall_0(SYS_fork);
	if (ret < 0)
		ret = -1;
	/* TODO: Set errno */

	return ret;
}

pid_t getpid(void)
{
	pid_t ret;

	ret = syscall_0(SYS_getpid);

	return ret;
}

ssize_t write(int fd, const void *buf, size_t count)
{
	unsigned long ret;

	ret = syscall_3(SYS_write, (unsigned long)fd, (unsigned long)buf,
		      (unsigned long)count);

	return (ssize_t)ret;
}
