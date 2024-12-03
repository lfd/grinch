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
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>

#include <grinch/grinch.h>
#include <grinch/vm.h>

#define CWD_BUF_GROWTH	32

pid_t create_grinch_vm(void)
{
	return syscall(SYS_grinch_create_grinch_vm);
}

int gcall(unsigned long no, unsigned long arg1)
{
	return syscall(SYS_grinch_call, no, arg1);
}
