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

#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <grinch/compiler_attributes.h>
#include <grinch/time_abi.h>

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#ifdef _GNU_SOURCE
extern char **environ;
#endif

void __noreturn exit(int status);
pid_t getpid(void);
pid_t fork(void);

int execve(const char *pathname, char *const argv[], char *const envp[]);

unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);

int close(int fd);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t read(int fd, void *buf, size_t count);

void *sbrk(intptr_t increment);
int brk(void *addr);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);

#endif /* _UNISTD_H */
