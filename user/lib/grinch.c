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
	return errno_syscall_0(SYS_grinch_create_grinch_vm);
}

int grinch_kstat(unsigned long no, unsigned long arg1)
{
	return errno_syscall_2(SYS_grinch_kstat, no, arg1);
}

char *grinch_getcwd(void)
{
	unsigned int sz;
	char *cwd, *tmp;

	sz = CWD_BUF_GROWTH;
	cwd = NULL;

again:
	tmp = realloc(cwd, sz);
	if (!tmp) {
		if (cwd)
			free(cwd);
		return NULL;
	}
	cwd = tmp;

	if (getcwd(cwd, sz) == NULL) {
		if (errno == ERANGE) {
			sz += CWD_BUF_GROWTH;
			goto again;
		}

		free(cwd);
		return NULL;
	}

	return cwd;
}
