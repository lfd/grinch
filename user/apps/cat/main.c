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
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv);

APP_NAME(cat);

static inline void cat_err(const char *pathname, int err)
{
	dprintf(stderr, "cat: %s: %pe\n", pathname, ERR_PTR(-errno));
}

static int cat(const char *pathname)
{
	ssize_t bytes_read, bytes_written;
	char buf[128];
	int fd;

	fd = open(pathname, O_RDONLY);
	if (fd == -1) {
		cat_err(pathname, -errno);
		return -errno;
	}

	do {
		bytes_read = read(fd, buf, sizeof(buf));
		if (bytes_read == -1) {
			cat_err(pathname, -errno);
			return -errno;
		}

		bytes_written = write(stdout, buf, bytes_read);
		if (bytes_written == -1) {
			cat_err(pathname, -errno);
			return -errno;
		}

		if (bytes_read != bytes_written) {
			cat_err(pathname, -EINVAL);
			return -EINVAL;
		}
	} while (bytes_read);

	return 0;
}

int main(int argc, char **argv)
{
	int arg, err;

	if (argc < 2)
		return -EINVAL;

	err = 0;
	printf_set_prefix(false);
	for (arg = 1; arg < argc; arg++)
		err |= cat(argv[arg]);

	return err ? -1 : 0;
}
