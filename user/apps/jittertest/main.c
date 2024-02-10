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

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define USLEEP	(1000 * 100)

int main(void);

APP_NAME(jittertest);

int main(void)
{
	unsigned long now, then;

	printf("Jittertest\n");

	for (;;) {
		now = gettime();
		usleep(USLEEP);
		then = gettime();

		now = (then - now) - USLEEP * 1000;
		printf("Jitter: %luus\n", now / 1000);
	}
	return 0;
}
