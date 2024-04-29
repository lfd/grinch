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

int cmain(int argc, char *argv[], char *envp[]);
int main(int argc, char *argv[], char *envp[]);

int cmain(int argc, char *argv[], char *envp[])
{
	environ = envp;
	return main(argc, argv, envp);
}
