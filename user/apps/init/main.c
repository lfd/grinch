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

#include "fb.h"

int main(void);

APP_NAME(init);

static char *const envp[] = {
	"PATH=/initrd/bin",
	NULL
};

static pid_t start_background(const char *path, char *const argv[], bool wait)
{
	pid_t child;
	int err, wstatus;

	printf("Starting %s\n", path);
	err = 0;
	child = fork();
	if (child == 0) {
		err = execve(path, argv, envp);
		perror("execve");
		exit(-errno);
	} else if (child == -1) {
		perror("fork");
		return -errno;
	}

	if (!wait)
		return child;

	err = waitpid(child, &wstatus, 0);
	if (err == -1) {
		perror("waitpid");
		return -errno;
	}

	if (err != child) {
		printf("waitpid: incorrect result\n");
		return -EINVAL;
	}

	if (WIFEXITED(wstatus))
		printf("Child %d: Exit code %d\n",
		       child, WEXITSTATUS(wstatus));
	else
		printf("Child %d: no regular exit\n", child);

	return 0;
}

int main(void)
{
	int wstatus;
	pid_t child;

	show_logo();

	start_background("/initrd/bin/gsh",
			 (char *[]){"gsh", NULL}, false);
	printf("Waiting for children...\n");

	while (true) {
		child = wait(&wstatus);
		if (child == -1) {
			perror("wait");
			if (errno == ECHILD) {
				printf("No more children!\n");
				break;
			}
			continue;
		}

		if (WIFEXITED(wstatus))
			printf("Child %d: Exit code %d\n",
			       child, WEXITSTATUS(wstatus));
		else
			printf("Child %d: no regular exit\n", child);
	}
	return 0;
}
