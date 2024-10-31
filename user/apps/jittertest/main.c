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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define USLEEP	(1000 * 100)
#define US_MAX	10000

int main(int argc, char *argv[]);

APP_NAME(jittertest);

static unsigned int histogram[US_MAX];
static unsigned int min = -1, max;
static unsigned long long avg;

/* return a measurement value in us */
static unsigned int single_measurement(void)
{
	unsigned long now, then;

	now = gettime();
	usleep(USLEEP);
	then = gettime();

	now = ((then - now) - USLEEP * 1000) / 1000;

	return now;
}

int main(int argc, char *argv[])
{
	unsigned long long max_shots, i, shots;
	unsigned int jitter;
	int err;

	printf_set_prefix(true);
	printf("Jittertest\n");

	if (argc > 2) {
		dprintf(STDERR_FILENO, "Invalid arguments!\n");
		return -EINVAL;
	} else if (argc == 2)
		max_shots = strtoull(argv[1], NULL, 0);
	else
		max_shots = -1;

	err = 0;
	for (shots = 0; shots < max_shots;) {
		jitter = single_measurement();
		shots++;
		avg += jitter;

		if (jitter < min)
			min = jitter;

		if (jitter > max)
			max = jitter;

		if (jitter >= US_MAX) {
			printf("Exceeded: %uus\n", jitter);
			err = -ERANGE;
			break;
		}

		histogram[jitter]++;

		printf("%5uus (min: %5uus avg: %5lluus max: %5uus)\n", jitter, min, avg / shots, max);
	}

	printf("Measured %llu shots:\n", shots);
	for (i = 0; i < US_MAX; i++)
		if (histogram[i])
			printf("%5lluus: %u\n", i, histogram[i]);

	return err;
}
