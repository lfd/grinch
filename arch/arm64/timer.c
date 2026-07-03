/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/init.h>
#include <grinch/timer.h>

timeu_t arch_timer_ticks_to_time(timeu_t ticks) { return 0; }
timeu_t arch_timer_get(void) { return 0; }
void arch_timer_set(timeu_t ns) {}
int __init arch_timer_init(void) { return 0; }
