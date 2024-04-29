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

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv);

APP_NAME(env);

int main(int argc, char **argv)
{
	char **envp;

	for (envp = environ; *envp; envp++)
		printf("%s\n", *envp);

	return 0;
}
