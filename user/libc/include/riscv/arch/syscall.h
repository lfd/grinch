/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2026
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
	asm volatile("ecall" : "=r"(a0) : "r"(a7), ##__VA_ARGS__ : "memory");

static __always_inline long __syscall0(long no)
{
	register long a7 asm("a7") = no;
	register long a0 asm("a0");

	__SYSCALL();

	return a0;
}

static __always_inline long __syscall1(long no, long arg0)
{
	register long a7 asm("a7") = no;
	register long a0 asm("a0") = arg0;

	__SYSCALL("0"(a0));

	return a0;
}

static __always_inline long __syscall2(long no, long arg0, long arg1)
{
	register long a7 asm("a7") = no;
	register long a0 asm("a0") = arg0;
	register long a1 asm("a1") = arg1;

	__SYSCALL("0"(a0), "r"(a1));

	return a0;
}

static __always_inline long __syscall3(long no, long arg0, long arg1, long arg2)
{
	register long a7 asm("a7") = no;
	register long a0 asm("a0") = arg0;
	register long a1 asm("a1") = arg1;
	register long a2 asm("a2") = arg2;

	__SYSCALL("0"(a0), "r"(a1), "r"(a2));

	return a0;
}

static __always_inline long __syscall4(long no, long arg0, long arg1, long arg2, long arg3)
{
	register long a7 asm("a7") = no;
	register long a0 asm("a0") = arg0;
	register long a1 asm("a1") = arg1;
	register long a2 asm("a2") = arg2;
	register long a3 asm("a3") = arg3;

	__SYSCALL("0"(a0), "r"(a1), "r"(a2), "r"(a3));

	return a0;
}

static __always_inline long __syscall5(long no, long arg0, long arg1, long arg2,
				       long arg3, long arg4)
{
	register long a7 asm("a7") = no;
	register long a0 asm("a0") = arg0;
	register long a1 asm("a1") = arg1;
	register long a2 asm("a2") = arg2;
	register long a3 asm("a3") = arg3;
	register long a4 asm("a4") = arg4;

	__SYSCALL("0"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4));

	return a0;
}

static __always_inline long __syscall6(long no, long arg0, long arg1, long arg2,
				       long arg3, long arg4, long arg5)
{
	register long a7 asm("a7") = no;
	register long a0 asm("a0") = arg0;
	register long a1 asm("a1") = arg1;
	register long a2 asm("a2") = arg2;
	register long a3 asm("a3") = arg3;
	register long a4 asm("a4") = arg4;
	register long a5 asm("a5") = arg5;

	__SYSCALL("0"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5));

	return a0;
}

#endif /* _ARCH_SYSCALL_H */
