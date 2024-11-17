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

#include <errno.h>

#include <generated/syscall.h>
#include <arch/syscall.h>

static inline long __syscall_ret(unsigned long err)
{
	if (err > -4096UL) {
		errno = -err;
		return -1;
	}

	return err;
}

#define __scast(X)	((long)(X))

#define __syscall1(n, arg0)			__syscall1(n, __scast(arg0))
#define __syscall2(n, arg0, arg1)		__syscall2(n, __scast(arg0), __scast(arg1))
#define __syscall3(n, arg0, arg1, arg2)		__syscall3(n, __scast(arg0), __scast(arg1), __scast(arg2))
#define __syscall4(n, arg0, arg1, arg2, arg3)	__syscall4(n, __scast(arg0), __scast(arg1), __scast(arg2), __scast(arg3))

#define __SYSCALL_NARGS_X(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, n,...) n
#define __SYSCALL_NARGS(...)	__SYSCALL_NARGS_X(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1, 0,)
#define __SYSCALL_NO_X(a,b)	a##b
#define __SYSCALL_NO(no)	__SYSCALL_NO_X(__syscall, no)

/* Calls the syscall, and returns the result as long */
#define __syscall(...)	__SYSCALL_NO(__SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

/* Calls the syscall, fills errno, and returns either 0 or -1 as int */
#define syscall(...)	__syscall_ret(__syscall(__VA_ARGS__))

#endif /* _SYSCALL_H */
