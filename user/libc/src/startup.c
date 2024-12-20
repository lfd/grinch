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

#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <_internal.h>

#include <grinch/elf.h>
#include <_internal.h>

int cmain(long *p);
int main(int argc, char *argv[], char *envp[]);

struct __libc __libc;

int __noreturn cmain(long *p)
{
	int argc, err;
	char **argv;
	size_t i;

	argc = p[0];
	argv = (void *)(p + 1);

	err = heap_init();
	if (err)
		goto out;

	environ = argv + argc + 1;

	for (i = 0; environ[i]; i++);
	__libc.auxv = (void *)(environ + i + 1);

	__libc.kinfo = (struct kinfo *)(uintptr_t)getauxval(AT_KINFO);
	if (!__libc.kinfo) {
		err = -ENOENT;
		goto out;
	}

	err = main(argc, argv, environ);

out:
	exit(err);
}
