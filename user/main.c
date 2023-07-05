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

int main(void)
{
	pid_t p;

	puts("Hello, world from userspace!\n");
	printf("My PID: %u\n", getpid());

#if 0
	const char *txt;
	p = fork();
	if (p == -1) {
		printf("Error forking\n");
		return -1;
	}

	txt = p == 0 ? "child" : "parent";
	for (;;) {
		printf("Hello from %s, PID %u\n", txt, getpid());
		sched_yield();

		if (getpid() == 2)
			exit(-1);
	}
#else
again:
	p = fork();
	if (p == 0)
		goto again;
	printf("Exiting from parent %u\n", getpid());
	exit(-42);
#endif

	return 0;
}
