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

#include <grinch/compiler_attributes.h>
#include <grinch/time.h>

static inline void
set_normalized_ts(struct timespec *ts, time_t sec, time_t nsec)
{
	while (nsec >= NSEC_PER_SEC) {
		nsec -= NSEC_PER_SEC;
		sec++;
	}

	while (nsec < 0) {
		nsec += NSEC_PER_SEC;
		sec--;
	}

	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

struct timespec timespec_add(struct timespec lhs, struct timespec rhs)
{
	struct timespec ts;

	set_normalized_ts(&ts,
			  lhs.tv_sec + rhs.tv_sec,
			  lhs.tv_nsec + rhs.tv_nsec);

	return ts;
}

void ns_to_ts(time_t ns, struct timespec *ts)
{
	if (likely(ns > 0)) {
		ts->tv_sec = ns / NSEC_PER_SEC;
		ts->tv_nsec = ns % NSEC_PER_SEC;
	} else {
		ts->tv_sec = -((-ns) / NSEC_PER_SEC) - 1;
		ts->tv_nsec = NSEC_PER_SEC - ((-ns) % NSEC_PER_SEC) - 1;
	}
}
