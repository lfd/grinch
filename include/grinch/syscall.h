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

#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <grinch/stat.h>

int syscall(unsigned long no, unsigned long arg1,
	    unsigned long arg2, unsigned long arg3,
	    unsigned long arg4, unsigned long arg5,
	    unsigned long arg6);

unsigned long sys_open(const char __user *path, int oflag);
unsigned long sys_close(int fd);
unsigned long sys_write(int fd, const char __user *buf, size_t count);
unsigned long sys_read(int fd, char __user *buf, size_t count);
int sys_execve(const char __user *pathname, char *const __user argv[],
	       char *const __user envp[]);
int sys_stat(const char __user *pathname, struct stat __user *st);

#endif /* _SYSCALL_H */
