/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/errno.h>

#define ERRNAME(err)	[err] = "-" #err

static const char *errnames[] = {
	ERRNAME(EPERM),
	ERRNAME(ENOENT),
	ERRNAME(EIO),
	ERRNAME(E2BIG),
	ERRNAME(EBADF),
	ERRNAME(ECHILD),
	ERRNAME(EAGAIN),
	ERRNAME(ENOMEM),
	ERRNAME(EFAULT),
	ERRNAME(EBUSY),
	ERRNAME(EEXIST),
	ERRNAME(ENODEV),
	ERRNAME(ENOTDIR),
	ERRNAME(EINVAL),
	ERRNAME(ERANGE),
	ERRNAME(ENOSYS),
	ERRNAME(EMSGSIZE),
};

const char *errname(int err)
{
	unsigned int no;

	if (err < 0)
		no = -err;
	else
		no = err;

	if (no < ARRAY_SIZE(errnames))
		return errnames[no];

	return NULL;
}
