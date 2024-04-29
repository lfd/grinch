/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <stdio.h>

int main(int argc, char **argv);

APP_NAME(echo);

int main(int argc, char **argv)
{
	int arg;

	for (arg = 1; arg < argc; arg++) {
		printf("%s%c", argv[arg], argv[arg + 1] ? ' ' : '\0');
	}
	printf("\n");

	return 0;
}
