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
#include <grinch/vm.h>

pid_t create_grinch_vm(void)
{
	int ret;

	ret = syscall_0(SYS_create_grinch_vm);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}
