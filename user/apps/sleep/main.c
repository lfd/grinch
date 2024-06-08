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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]);

APP_NAME(sleep);

int main(int argc, char *argv[])
{
	unsigned long sec;

	if (argc != 2)
		return -EINVAL;

	sec = strtoul(argv[1], NULL, 0);
	sleep(sec);

	return 0;
}
