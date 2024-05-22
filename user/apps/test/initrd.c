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

#include "vfs.h"

#define FNAME	"/initrd/test.txt"

int test_initrd(void)
{
	unsigned int i;
	int err, fd;
	char buf[64];
	ssize_t r;
	char *tmp;

	fd = open(FNAME, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	r = read(fd, buf, sizeof(buf));
	if (r == -1) {
		perror("read");
		return -errno;
	}

	printf("  -> read %lu bytes: %s", r, buf);

	err = close(fd);
	if (err) {
		perror("close");
		return -errno;
	}

	/* And read 1 byte-wise */
	fd = open(FNAME, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -errno;
	}

	memset(buf, 0, sizeof(buf));
	tmp = buf;
	i = 0;
	for (i = 0; ; i++, tmp++) {
		r = read(fd, tmp, 1);
		if (r == -1) {
			perror("read");
			err = -errno;
			goto close_out;
		}

		if (!r)
			break;
	}

	printf("  -> read %u bytes: %s", i, buf);
	err = 0;

close_out:
	close(fd);

	return err;
}
