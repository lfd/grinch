/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
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

	puts("Hello, world from userspace!\n");
	printf("My PID: %u\n", getpid());

	for (forked = 0; forked < 5; forked++) {
		printf("PID %u: Forking\n", getpid());
		p = fork();
		if (p == 0) { /* child */
			printf("PID %u: Calling execve...\n", getpid());
			execve("initrd:/hello.echse", NULL, NULL);
			printf("PID %u: Execve Error occured!\n", getpid());
		} else if (p == -1) { /* error */
			printf("PID %u: Fork error!\n", getpid());
			exit(-5);
		} else { /* parent */
			printf("PID %u: Parent returned. Created PID %u.\n", getpid(), p);
		}
	}

	return 0;
}
