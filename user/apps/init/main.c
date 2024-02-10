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

#include <sched.h>
#include <stdio.h>
#include <unistd.h>

int main(void);

APP_NAME(init);

int main(void)
{
	int forked;
	pid_t p;

	puts("Starting Jittertest...\n");
	p = fork();
	if (p == 0) {
		execve("initrd:/jittertest.echse", NULL, NULL);
		printf("Execve returned\n");
		exit(-1);
	} else if (p == -1) {
		printf("Fork error!\n");
		exit(-1);
	}

	for (forked = 0; forked < 5; forked++) {
		puts("Forking...\n");
		p = fork();
		if (p == 0) { /* child */
			printf("Calling execve...\n");
			execve("initrd:/hello.echse", NULL, NULL);
			printf("Execve returned\n");
		} else if (p == -1) { /* error */
			printf("Fork error!\n");
			exit(-5);
		} else { /* parent */
			printf("Parent returned. Created PID %u.\n", p);
		}
	}

	return 0;
}
