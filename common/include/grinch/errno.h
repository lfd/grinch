/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _GRINCH_ERRNO_H
#define _GRINCH_ERRNO_H

#include <grinch/compiler_attributes.h>
#include <grinch/types.h>

#define EPERM		1
#define ENOENT		2
#define EIO		5
#define E2BIG		7
#define EBADF		9
#define ECHILD		10
#define EAGAIN		11
#define EWOULDBLOCK	EAGAIN
#define ENOMEM		12
#define EFAULT		14
#define EBUSY		16
#define EEXIST		17
#define ENODEV		19
#define ENOTDIR		20
#define EISDIR		21
#define EINVAL		22
#define EFBIG		27
#define EROFS		30
#define ERANGE		34
#define ENOSYS		38
#define ENAMETOOLONG	78
#define	EMSGSIZE	97

#define MAX_ERRNO	97

#define IS_ERR_VALUE(x)	unlikely((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static __always_inline void *ERR_PTR(long error)
{
	return (void *)error;
}

static __always_inline long PTR_ERR(const void *ptr)
{
	return (long)ptr;
}

static __always_inline bool IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static __always_inline void *ERR_CAST(const void *ptr)
{
		return (void *) ptr;
}

const char *errname(int err);

#endif /* _GRINCH_ERRNO_H */
