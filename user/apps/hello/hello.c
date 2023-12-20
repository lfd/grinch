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

APP_NAME(hello);

int main(void)
{
	printf("Hello, world! (PID %u)\n", getpid());
	//sched_yield();
	return 0;
}
