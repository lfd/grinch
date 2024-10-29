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

struct sbiret {
	long error;
	long value;
};

static __always_inline struct sbiret
sbi_ecall(int ext, unsigned long arg0, unsigned long arg1, unsigned long arg2,
	  unsigned long arg3, unsigned long arg4, unsigned long arg5)
{
	struct sbiret ret;

	register unsigned long a0 asm ("a0") = arg0;
	register unsigned long a1 asm ("a1") = arg1;
	register unsigned long a2 asm ("a2") = arg2;
	register unsigned long a3 asm ("a3") = arg3;
	register unsigned long a4 asm ("a4") = arg4;
	register unsigned long a5 asm ("a5") = arg5;
	register unsigned long a7 asm ("a7") = ext;
	asm volatile ("ecall"
		: "+r" (a0), "+r" (a1)
		: "r" (a2), "r" (a3), "r" (a4),
		  "r" (a5), "r" (a7)
		: "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

static __always_inline
unsigned long syscall(unsigned long no, unsigned long arg0,
		      unsigned long arg1, unsigned long arg2,
		      unsigned long arg3, unsigned long arg4,
		      unsigned long arg5)
{
	struct sbiret ret;

	ret = sbi_ecall(no, arg0, arg1, arg2, arg3, arg4, arg5);

	return ret.error;
}

#endif /* _ARCH_SYSCALL_H */
