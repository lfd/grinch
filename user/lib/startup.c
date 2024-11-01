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

#include <stdlib.h>
#include <unistd.h>

int cmain(int argc, char *argv[], char *envp[]);
int main(int argc, char *argv[], char *envp[]);

int __noreturn cmain(int argc, char *argv[], char *envp[])
{
	int err;

	err = heap_init();
	if (err)
		goto out;

	environ = envp;
	err = main(argc, argv, envp);

out:
	exit(err);
}
