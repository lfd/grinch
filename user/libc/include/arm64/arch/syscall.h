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

#ifndef _ARCH_SYSCALL_H
#define _ARCH_SYSCALL_H

#define __SYSCALL(...)	\
	asm volatile("svc #0" : "=r"(x0) : "r"(x8), ##__VA_ARGS__ : "memory")

static __always_inline long __syscall0(long no)
{
	register long x8 asm("x8") = no;
	register long x0 asm("x0");

	__SYSCALL();

	return x0;
}

static __always_inline long __syscall1(long no, long arg0)
{
	register long x8 asm("x8") = no;
	register long x0 asm("x0") = arg0;

	__SYSCALL("0"(x0));

	return x0;
}

static __always_inline long __syscall2(long no, long arg0, long arg1)
{
	register long x8 asm("x8") = no;
	register long x0 asm("x0") = arg0;
	register long x1 asm("x1") = arg1;

	__SYSCALL("0"(x0), "r"(x1));

	return x0;
}

static __always_inline long __syscall3(long no, long arg0, long arg1, long arg2)
{
	register long x8 asm("x8") = no;
	register long x0 asm("x0") = arg0;
	register long x1 asm("x1") = arg1;
	register long x2 asm("x2") = arg2;

	__SYSCALL("0"(x0), "r"(x1), "r"(x2));

	return x0;
}

#endif /* _ARCH_SYSCALL_H */
