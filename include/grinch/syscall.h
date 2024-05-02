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

#include <grinch/dirent.h>
#include <grinch/stat.h>

void syscall(unsigned long no, unsigned long arg1,
	     unsigned long arg2, unsigned long arg3,
	     unsigned long arg4, unsigned long arg5,
	     unsigned long arg6);

long sys_fork(void);
long sys_open(const char __user *path, int oflag);
long sys_close(int fd);
long sys_write(int fd, const char __user *buf, size_t count);
long sys_read(int fd, char __user *buf, size_t count);
long sys_execve(const char __user *pathname, char *const __user argv[],
		char *const __user envp[]);
long sys_stat(const char __user *pathname, struct stat __user *st);
long sys_getdents(int fd, struct grinch_dirent __user *dents, unsigned int size);
long sys_brk(unsigned long addr);
long sys_wait(pid_t pid, int __user *wstatus, int options);

/* custom syscalls */
long sys_grinch_create_grinch_vm(void);

#endif /* _SYSCALL_H */
