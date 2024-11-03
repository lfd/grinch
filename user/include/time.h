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

#ifndef _TIME_H
#define _TIME_H

#include <grinch/time_abi.h>

int clock_gettime(clockid_t clockid, struct timespec *ts);

#endif /* _TIME_H */
