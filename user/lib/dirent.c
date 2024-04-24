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

#include <dirent.h>

#include <errno.h>
#include <syscall.h>

int getdents(int fd, struct grinch_dirent *dents, unsigned int size)
{
	return errno_syscall_3(SYS_getdents, fd, (unsigned long)dents, size);
}
