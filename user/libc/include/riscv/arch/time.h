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

#ifndef _ARCH_TIME_H
#define _ARCH_TIME_H

#include <arch/csr.h>
#include <grinch/div64.h>
#include <_internal.h>

#define NSEC_PER_SEC   (1000L * 1000L * 1000L)

static inline timeu_t get_time(void)
{
#if ARCH_RISCV == 64 /* rv64 */
	return csr_read(time);
#elif ARCH_RISCV == 32 /* rv32 */
	u32 hi, lo;
	do {
		hi = csr_read(timeh);
		lo = csr_read(time);
	} while (hi != csr_read(timeh));

	return ((u64)hi << 32) | lo;
#endif
}

static inline int arch_clock_gettime(clockid_t clockid, struct timespec *ts)
{
	timeu_t ns;
	u32 rem;

	if (clockid != 0)
		return -EINVAL;

	ns = NSEC_PER_SEC * get_time();
	do_div(ns, __libc.kinfo->riscv.timebase_frequency);
	ns -= __libc.kinfo->wall_base;

	ts->tv_sec = div_u64_rem(ns, NSEC_PER_SEC, &rem);
	ts->tv_nsec = rem;

	return 0;

}

#endif /* _ARCH_TIME_H */
