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
#include <grinch/grinch.h>
#include <grinch/vm.h>

pid_t create_grinch_vm(void)
{
	return errno_syscall_0(SYS_create_grinch_vm);
}

int grinch_kstat(unsigned long no, unsigned long arg1)
{
	return errno_syscall_2(SYS_grinch_kstat, no, arg1);
}
