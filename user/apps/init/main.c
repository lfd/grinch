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

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

int main(void);

APP_NAME(init);

static int start_background(const char *path)
{
	pid_t child;
	int err;

	printf("Starting %s\n", path);
	err = 0;
	child = fork();
	if (child == 0) {
		err = execve(path, NULL, NULL);
	} else if (child == -1) {
		perror("fork");
		err = -errno;
	}

	return err;
}

int main(void)
{
	int err, forked;

	err = start_background("/initrd/test.echse");
	if (err)
		return err;

	err = start_background("/initrd/jittertest.echse");
	if (err)
		return err;

	for (forked = 0; forked < 5; forked++) {
		err = start_background("/initrd/hello.echse");
		if (err)
			return err;
	}

	return 0;
}
