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
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

#include "syscalls.h"

#define NO_FORKS	50

static int test_fork(void)
{
	unsigned int i;
	pid_t child;
	int err;
	bool failed;

	failed = false;
	for (i = 0; i < NO_FORKS; i++) {
		child = fork();
		if (child == 0) {
			err = execve("/initrd/bin/true", NULL, NULL);
			perror("execve");
			failed = true;
			break;
		} else if (child == -1) {
			perror("fork!");
			failed = true;
			break;
		}
	}

	for (i = 0; ; i++) {
		child = wait(NULL);
		if (child == -1) {
			if (errno != ECHILD) {
				perror("wait");
				failed = true;
			}
			break;
		}
	}

	if (failed) {
		printf("Test failed\n");
		return -EINVAL;
	}

	err = (i == NO_FORKS) ? 0 : -EINVAL;

	return err;
}

int test_syscalls(void)
{
	int err;

	printf("Testing fork+wait\n");
	err = test_fork();
	if (err)
		return err;

	return err;
}
