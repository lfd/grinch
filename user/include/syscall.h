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

#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <errno.h>

#include <generated/syscall.h>
#include <arch/syscall.h>

#define syscall_0(no)				\
	syscall((no), 0, 0, 0, 0, 0, 0)
#define syscall_1(no, arg1)			\
	syscall((no), (arg1), 0, 0, 0, 0, 0)
#define syscall_2(no, arg1, arg2)		\
	syscall((no), (arg1), (arg2), 0, 0, 0, 0)
#define syscall_3(no, arg1, arg2, arg3)		\
	syscall((no), (arg1), (arg2), (arg3), 0, 0, 0)
#define syscall_4(no, arg1, arg2, arg3, arg4)	\
	syscall((no), (arg1), (arg2), (arg3), (arg4), 0, 0)

static inline
unsigned long errno_syscall(unsigned long no,
			    unsigned long arg1,
			    unsigned long arg2,
			    unsigned long arg3)
{
	long ret;

	ret = syscall_3(no, arg1, arg2, arg3);
	if (ret >= 0)
		return ret;

	errno = -ret;
	return -1;
}

#define errno_syscall_0(no)			errno_syscall(no, 0, 0, 0)
#define errno_syscall_1(no, arg1)		errno_syscall(no, arg1, 0, 0)
#define errno_syscall_2(no, arg1, arg2)		errno_syscall(no, arg1, arg2, 0)
#define errno_syscall_3(no, arg1, arg2, arg3)	errno_syscall(no, arg1, arg2, arg3)

#endif /* _SYSCALL_H */
