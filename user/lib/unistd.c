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

#include <errno.h>
#include <syscall.h>
#include <unistd.h>

void __noreturn exit(int status)
{
	syscall_1(SYS_exit, status);
	for (;;);
	__builtin_unreachable();
}

pid_t fork(void)
{
	return errno_syscall_0(SYS_fork);
}

pid_t getpid(void)
{
	return errno_syscall_0(SYS_getpid);
}

int usleep(unsigned int usec)
{
	return errno_syscall_1(SYS_usleep, usec);
}

unsigned int sleep(unsigned int seconds)
{
	int err;

	err = usleep(seconds * 1000 * 1000);
	if (!err)
		return seconds;
	return 0;
}

ssize_t read(int fd, void *buf, size_t count)
{
	return errno_syscall_3(SYS_read, (unsigned long)fd, (unsigned long)buf,
			       (unsigned long)count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return errno_syscall_3(SYS_write, (unsigned long)fd,
			       (unsigned long)buf, (unsigned long)count);
}

int execve(const char *pathname, char *const argv[], char *const envp[])
{
	int ret;
	ret = syscall_3(SYS_execve, (unsigned long)pathname,
			(unsigned long)argv, (unsigned long)envp);

	errno = -ret;

	return -1;
}

int close(int fd)
{
	return errno_syscall_1(SYS_close, fd);
}
