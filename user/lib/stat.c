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

#include <sys/stat.h>

#include <errno.h>
#include <syscall.h>

int stat(const char *pathname, struct stat *statbuf)
{
	return syscall(SYS_stat, pathname, statbuf);
}

int mkdir(const char *pathname, mode_t mode)
{
	return syscall(SYS_mkdir, pathname, mode);
}
