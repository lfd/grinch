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

#include <asm-generic/paging.h>

#include <errno.h>
#include <syscall.h>
#include <stdio.h>
#include <unistd.h>

static void *curbrk;

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
	return errno_syscall_1(SYS_grinch_usleep, usec);
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

static inline void *__brk(void *addr)
{
	return (void *)syscall_1(SYS_brk, (unsigned long)addr);
}

int brk(void *addr)
{
	void *new;

	curbrk = new = __brk(addr);
	if (IS_ERR(new)) {
		errno = -PTR_ERR(new);
		return -1;
	}

	return 0;
}

void *sbrk(intptr_t increment)
{
	void *oldbrk;

	if (increment % PAGE_SIZE) {
		dprintf(stderr, "Invalid sbrk()\n");
		exit(-EINVAL);
	}

	if (!curbrk) {
		if (brk(0) < 0)
			return (void *)-1;
	}

	if (!increment)
		return curbrk;

	oldbrk = curbrk;
	if (increment > 0
	    ? ((uintptr_t)oldbrk + (uintptr_t)increment < (uintptr_t)oldbrk)
	    : ((uintptr_t)oldbrk < (uintptr_t)-increment)) {
		errno = -ENOMEM;
		return (void *)-1;
	}

	if (brk(oldbrk + increment) < 0)
		return (void *)-1;

	return oldbrk;
}

char *getcwd(char *buf, size_t size)
{
	int ret;

	ret = errno_syscall_2(SYS_getcwd, (unsigned long)buf, size);
	if (ret == 0)
		return buf;

	return NULL;
}

int chdir(const char *path)
{
	return errno_syscall_1(SYS_chdir, (unsigned long)path);
}
