/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
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

#define NO_SCHED_CHILDREN	16
#define NO_SCHED_ROUNDS		4096
#define SCHED_YIELD_EVERY	7
#define SCHED_SLEEP_EVERY	257

int main(void);

static int sched_child(unsigned int child_no)
{
	const struct timespec nap = {
		.tv_sec = 0,
		.tv_nsec = 1000,
	};
	volatile unsigned long acc;
	unsigned int i;
	int err;

	acc = (unsigned long)getpid() ^ child_no;

	for (i = 0; i < NO_SCHED_ROUNDS; i++) {
		acc = (acc * 1103515245UL) + 12345UL + i;

		if ((i % SCHED_YIELD_EVERY) == 0) {
			err = sched_yield();
			if (err) {
				perror("sched_yield");
				return -errno;
			}
		}

		if ((i % SCHED_SLEEP_EVERY) == 0) {
			err = nanosleep(&nap, NULL);
			if (err) {
				perror("nanosleep");
				return -errno;
			}
		}
	}

	return acc ? 0 : -EINVAL;
}

int main(void)
{
	unsigned int created, reaped;
	int status, err;
	pid_t child;

	for (created = 0; created < NO_SCHED_CHILDREN; created++) {
		child = fork();
		if (child == 0)
			exit(sched_child(created) ? 1 : 0);

		if (child == -1) {
			perror("fork");
			err = -errno;
			goto wait_out;
		}
	}

	err = 0;

wait_out:
	for (reaped = 0; ; reaped++) {
		child = wait(&status);
		if (child == -1) {
			if (errno != ECHILD) {
				perror("wait");
				return -errno;
			}
			break;
		}

		if (!WIFEXITED(status) || WEXITSTATUS(status)) {
			printf("child %d failed with status %d\n", child, status);
			err = -EINVAL;
		}
	}

	if (reaped != created) {
		printf("reaped %u/%u scheduler children\n", reaped, created);
		return -EINVAL;
	}

	return err;
}
