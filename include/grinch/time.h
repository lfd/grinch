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

/* Inspired by and adapted from the Linux kernel sources */

#ifndef _GRINCH_TIME_H
#define _GRINCH_TIME_H

#include <grinch/time_abi.h>

#define US_TO_NS(x)	((x) * 1000L)
#define MS_TO_NS(x)	(US_TO_NS((x) * 1000L))
#define S_TO_NS(x)	(MS_TO_NS((x) * 1000L))

#define NSEC_PER_SEC	S_TO_NS(1)
#define MSEC_PER_SEC	MS_TO_NS(1)
#define USEC_PER_SEC	US_TO_NS(1)

#define TIME_MAX	((s64)~((u64)1 << 63))
#define TIME_MIN	(-TIME_MAX - 1)
#define TIME_SEC_MAX	(TIME_MAX / NSEC_PER_SEC)
#define TIME_SEC_MIN	(TIME_MIN / NSEC_PER_SEC)

#define HZ_TO_NS(x)	(NSEC_PER_SEC / (x))

#define PR_TIME_FMT		"%04llu.%03llu"
#define PR_TIME_PARAMS(x)	((x) / NSEC_PER_SEC), ((((x) * 1000) / NSEC_PER_SEC) % 1000)

void ns_to_ts(time_t ns, struct timespec *ts);
static inline void us_to_ts(time_t ns, struct timespec *ts)
{
	return ns_to_ts(ns * 1000, ts);
}

static inline time_t ts_to_ns(struct timespec *ts)
{
	if (ts->tv_sec >= TIME_SEC_MAX)
		return TIME_MAX;

	if (ts->tv_sec <= TIME_SEC_MIN)
		return TIME_MIN;

	return (ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}

struct timespec timespec_add(struct timespec lhs, struct timespec rhs);

#endif /* _GRINCH_TIME_H */
