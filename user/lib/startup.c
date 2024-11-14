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
#include <stdlib.h>
#include <unistd.h>

int cmain(long *p);
int main(int argc, char *argv[], char *envp[]);

int __noreturn cmain(long *p)
{
	int argc, err;
	char **argv;

	argc = p[0];
	argv = (void *)(p + 1);

	err = heap_init();
	if (err)
		goto out;

	environ = argv + argc + 1;
	err = main(argc, argv, environ);

out:
	exit(err);
}
