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
	const char *txt;
	pid_t p;

	puts("Hello, world from userspace!\n");
	printf("My PID: %u\n", getpid());

	p = fork();
	if (p == -1) {
		printf("Error forking\n");
		return -1;
	}

	txt = p == 0 ? "child" : "parent";
	for (;;) {
		printf("Hello from %s, PID %u\n", txt, getpid());
		sched_yield();
	}

	return 0;
}
