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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv);

int main(int argc, char **argv)
{
	const char *filename;
	int fd;

	if (argc != 2)
		return -EINVAL;

	filename = argv[1];

	fd = open(filename, O_CREAT);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	close(fd);

	return EXIT_SUCCESS;
}
