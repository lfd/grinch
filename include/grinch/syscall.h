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

/* Inspired by Linux */

#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <grinch/dirent.h>
#include <grinch/stat.h>

struct syscall_args {
	unsigned long arg1;
	unsigned long arg2;
	unsigned long arg3;
	unsigned long arg4;
	unsigned long arg5;
	unsigned long arg6;
};

typedef long (*syscall_stub_t)(struct syscall_args *args);

#define __SC_DECL(t,a)		t a
#define __SC_CA(type, no)	(type)(___a->arg##no)

#define __MAP0(m, ...)		m(void,)
#define __MAP1(m, t, a,...)	m(t, a)
#define __MAP2(m, t, a,...)	m(t, a), __MAP1(m, __VA_ARGS__)
#define __MAP3(m, t, a,...)	m(t, a), __MAP2(m, __VA_ARGS__)
#define __MAP4(m, t, a,...)	m(t, a), __MAP3(m, __VA_ARGS__)
#define __MAP(n, ...)		__MAP##n(__VA_ARGS__)

#define __MAPARGS0(...)
#define __MAPARGS1(t1, a1)			\
	__SC_CA(t1, 1)
#define __MAPARGS2(t1, a1, t2, a2)		\
	__MAPARGS1(t1, a1), __SC_CA(t2, 2)
#define __MAPARGS3(t1, a1, t2, a2, t3, a3)	\
	__MAPARGS2(t1, a1, t2, a2), __SC_CA(t3, 3)
#define __MAPARGS4(t1, a1, t2, a2, t3, a3, t4, a4)	\
	__MAPARGS3(t1, a1, t2, a2, t3, a3), __SC_CA(t4, 4)
#define __MAPARGS(n, ...)			__MAPARGS##n(__VA_ARGS__)

#define SC_STUB_PROTO(name)	long ___sys_##name(struct syscall_args *___a)

#define SC_PROTO(name, no_args, ...)	\
	static __always_inline long \
	sys_##name(__MAP(no_args, __SC_DECL, __VA_ARGS__))

#define SYSCALL_DEFx(name, no_args, ...)				\
	SC_STUB_PROTO(name);						\
	SC_PROTO(name, no_args, __VA_ARGS__);				\
	SC_STUB_PROTO(name)						\
	{								\
		return sys_##name(__MAPARGS(no_args, __VA_ARGS__));	\
	}								\
	SC_PROTO(name, no_args, __VA_ARGS__)

#define SYSCALL_DEF0(name, ...)	SYSCALL_DEFx(name, 0, __VA_ARGS__)
#define SYSCALL_DEF1(name, ...)	SYSCALL_DEFx(name, 1, __VA_ARGS__)
#define SYSCALL_DEF2(name, ...)	SYSCALL_DEFx(name, 2, __VA_ARGS__)
#define SYSCALL_DEF3(name, ...)	SYSCALL_DEFx(name, 3, __VA_ARGS__)
#define SYSCALL_DEF4(name, ...)	SYSCALL_DEFx(name, 4, __VA_ARGS__)

void syscall(unsigned long no, struct syscall_args *args);

#endif /* _SYSCALL_H */
