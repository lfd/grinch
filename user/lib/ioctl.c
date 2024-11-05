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

#include <errno.h>
#include <syscall.h>
#include <stdarg.h>

#include <sys/ioctl.h>

int ioctl(int fd, int op, ...)
{
	va_list ap;
	void *arg;

	va_start(ap, op);
	arg = va_arg(ap, void *);
	va_end(ap);

	return errno_syscall_3(SYS_ioctl, fd, op, (unsigned long)arg);
}
