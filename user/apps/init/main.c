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

#include <sys/wait.h>

#include <grinch/vm.h>

int main(void);

APP_NAME(init);

static pid_t start_background(const char *path, bool wait)
{
	pid_t child;
	int err;

	printf("Starting %s\n", path);
	err = 0;
	child = fork();
	if (child == 0) {
		err = execve(path, NULL, NULL);
		perror("execve");
		exit(-errno);
	} else if (child == -1) {
		perror("fork");
		return -errno;
	}

	if (!wait)
		return child;

	err = waitpid(child, NULL, 0);
	if (err == -1) {
		perror("waitpid");
		return -errno;
	}

	if (err != child)
		return -EINVAL;

	return 0;
}

static int init(void)
{
	int forked;
	pid_t child;

	for (forked = 0; forked < 5; forked++) {
		child = start_background("/initrd/hello.echse", false);
		if (child < 0)
			return child;
	}

	child = start_background("/initrd/test.echse", true);
	if (child < 0)
		return child;

	child = start_background("/initrd/jittertest.echse", false);
	if (child < 0)
		return child;

	if (1) {
		child = create_grinch_vm();
		if (child == -1) {
			perror("create_grinch_vm");
			return -errno;
		} else
			printf("Created grinch VM with PID %d\n", child);
	}

	return 0;
}

int main(void)
{
	int err;
	pid_t child;

	err = init();
	printf("Init finished with %pe. Waiting.\n", ERR_PTR(err));

	while (true) {
		child = wait(NULL);
		if (child == -1)
			perror("wait");
		else
			printf("Collected child %d\n", child);
	}
	return 0;
}
