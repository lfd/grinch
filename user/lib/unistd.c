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

#include <unistd.h>
#include <syscall.h>

void __noreturn exit(int status)
{
	syscall_1(SYS_exit, status);
	for (;;);
	__builtin_unreachable();
}

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

int usleep(unsigned int usec)
{
	return syscall_1(SYS_usleep, usec);
}

unsigned int sleep(unsigned int seconds)
{
	int err;

	err = usleep(seconds * 1000 * 1000);
	if (!err)
		return seconds;
	return 0;
}

ssize_t write(int fd, const void *buf, size_t count)
{
	unsigned long ret;

	ret = syscall_3(SYS_write, (unsigned long)fd, (unsigned long)buf,
		      (unsigned long)count);

	return (ssize_t)ret;
}

int execve(const char *pathname, char *const argv[], char *const envp[])
{
	return syscall_3(SYS_execve, (unsigned long)pathname,
			 (unsigned long)argv, (unsigned long)envp);
}
