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

#ifndef _TIMER_H
#define _TIMER_H

#include <grinch/time.h>
#include <grinch/arch/timer.h>

int timer_init(void);

void handle_timer(void);

void timer_get_wall(struct timespec *ts);
timeu_t timer_get_wall_ns(void);

timeu_t arch_timer_ticks_to_time(timeu_t ticks);
void timer_ticks_to_time(timeu_t ticks, struct timespec *ts);

/* Architecture specific routines */
int arch_timer_init(void);

timeu_t arch_timer_get(void);
void arch_timer_set(timeu_t ns);

/* task_lock must be held */
#include <grinch/task.h>
void timer_update(struct task *task);

#endif /* _TIMER_H */
