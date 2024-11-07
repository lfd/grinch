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

#include <grinch/div64.h>

#define NSEC_PER_SEC		(1000L * 1000L * 1000L)
#define USLEEP			(1000 * 100)
#define HISTOGRAM_US_MAX	10000

int main(int argc, char *argv[]);

APP_NAME(jittertest);

static unsigned int histogram[HISTOGRAM_US_MAX];
static unsigned int min = -1, max;
static unsigned long long avg;

static inline time_t
calcdiff_ns(const struct timespec *t1, const struct timespec *t2)
{
	time_t diff;

	diff = NSEC_PER_SEC * (t1->tv_sec - t2->tv_sec);
	diff += t1->tv_nsec - t2->tv_nsec;

	return diff;
}

/* return a measurement value in us */
static int single_measurement(unsigned int *meas)
{
	struct timespec now, then;
	int err;

	err = clock_gettime(0, &now);
	if (err == -1)
		goto err;

	err = usleep(USLEEP);
	if (err == -1)
		goto err;

	err = clock_gettime(0, &then);
	if (err == -1)
		goto err;

	*meas = (unsigned int)(calcdiff_ns(&then, &now) - USLEEP * 1000) / 1000;
	return 0;

err:
	return errno ? -errno : -1;
}

int main(int argc, char *argv[])
{
	unsigned int i, jitter, shots;
	unsigned long max_shots;
	int err;

	printf_set_prefix(true);
	printf("Jittertest\n");

	if (argc > 2) {
		dprintf(STDERR_FILENO, "Invalid arguments!\n");
		return -EINVAL;
	} else if (argc == 2)
		max_shots = strtoul(argv[1], NULL, 0);
	else
		max_shots = -1;

	err = 0;
	for (shots = 0; shots < max_shots;) {
		err = single_measurement(&jitter);
		if (err)
			break;

		shots++;
		avg += jitter;

		if (jitter < min)
			min = jitter;

		if (jitter > max)
			max = jitter;

		if (jitter >= HISTOGRAM_US_MAX) {
			printf("Exceeded: %uus\n", jitter);
			err = -ERANGE;
			break;
		}

		histogram[jitter]++;

		printf("%5uus (min: %5uus avg: %5lluus max: %5uus)\n",
		       jitter, min, div_u64(avg, shots), max);
	}

	printf("Measured %u shots:\n", shots);
	for (i = 0; i < HISTOGRAM_US_MAX; i++)
		if (histogram[i])
			printf("%5uus: %u\n", i, histogram[i]);

	return err;
}
