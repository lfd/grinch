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

#ifndef _ERRNO_H
#define _ERRNO_H

#include <grinch/compiler_attributes.h>
#include <grinch/types.h>

#define EPERM		1
#define ENOENT		2
#define EIO		5
#define EBADF		9
#define E2BIG		7
#define ENOMEM		12
#define EFAULT		14
#define EBUSY		16
#define EEXIST		17
#define ENODEV		19
#define EINVAL		22
#define ERANGE		34
#define ENOSYS		38

#define MAX_ERRNO	38

#define IS_ERR_VALUE(x)	unlikely((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *)error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long)ptr;
}

static inline bool IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

const char *errname(int err);

#endif /* _ERRNO_H */
