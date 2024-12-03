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
#include <limits.h>
#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <grinch/div64.h>

static void *curbrk;

void __noreturn exit(int status)
{
	for (;;)
		syscall(SYS_exit, status);
}

pid_t fork(void)
{
	return syscall(SYS_fork);
}

pid_t getpid(void)
{
	return syscall(SYS_getpid);
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	return syscall(SYS_nanosleep, req, rem);
}

int usleep(useconds_t usec)
{
	struct timespec tv;
	u32 rem;

	tv.tv_sec = div_u64_rem(usec, 1000000, &rem);
	tv.tv_nsec = rem * 1000;

	return nanosleep(&tv, &tv);
}

unsigned int sleep(unsigned int seconds)
{
	struct timespec tv = {
		.tv_sec = seconds,
		.tv_nsec = 0,
	};

	if (nanosleep(&tv, &tv))
		return tv.tv_sec;

	return 0;
}

ssize_t read(int fd, void *buf, size_t count)
{
	return syscall(SYS_read, fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return syscall(SYS_write, fd, buf, count);
}

int execve(const char *pathname, char *const argv[], char *const envp[])
{
	return syscall(SYS_execve, pathname, argv, envp);
}

int close(int fd)
{
	return syscall(SYS_close, fd);
}

static inline void *__brk(void *addr)
{
	return (void *)__syscall(SYS_brk, addr);
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
		dprintf(STDERR_FILENO, "Invalid sbrk()\n");
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
		errno = ENOMEM;
		return (void *)-1;
	}

	if (brk(oldbrk + increment) < 0)
		return (void *)-1;

	return oldbrk;
}

char *getcwd(char *buf, size_t size)
{
	char tmp[buf ? 1 : PATH_MAX];
	int ret;

	/*
	 * looks a bit ugly, but otherwise the compiler will create an error
	 * with high optimisation level
	 */
	if (buf == NULL) {
		buf = tmp;
		size = sizeof(tmp);
		ret = syscall(SYS_getcwd, buf, size);
	} else if (!size) {
		errno = EINVAL;
		return NULL;
	} else
		ret = syscall(SYS_getcwd, buf, size);

	if (ret == -1)
		return NULL;

	if (buf[0] != '/') {
		errno = ENOENT;
		return NULL;
	}

	if (buf == tmp)
		return strdup(buf);

	return buf;
}

int chdir(const char *path)
{
	return syscall(SYS_chdir, path);
}
