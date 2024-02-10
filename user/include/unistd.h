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

typedef int pid_t;

void __noreturn exit(int status);
ssize_t write(int fd, const void *buf, size_t count);
pid_t getpid(void);
pid_t fork(void);

int execve(const char *pathname, char *const argv[], char *const envp[]);
unsigned int sleep(unsigned int seconds);
int usleep(unsigned int usec);

#endif /* _UNISTD_H */
