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

#define US_TO_NS(x)	((x) * 1000ULL)
#define MS_TO_NS(x)	(US_TO_NS((x) * 1000ULL))
#define S_TO_NS(x)	(MS_TO_NS((x) * 1000ULL))

#define NS		S_TO_NS(1)
#define MS		MS_TO_NS(1)
#define US		US_TO_NS(1)

#define HZ_TO_NS(x)	(NS / (x))

#define PR_TIME_FMT		"%04llu.%03llu"
#define PR_TIME_PARAMS(x)	((x) / NS), ((((x) * 1000) / NS) % 1000)

int timer_init(void);

void handle_timer(void);

unsigned long timer_get_wall(void);
unsigned long arch_timer_ticks_to_time(unsigned long ticks);
unsigned long timer_ticks_to_time(unsigned long ticks);

/* Architecture specific routines */
int arch_timer_init(void);

unsigned long arch_timer_get(void);
void arch_timer_set(unsigned long ns);

#endif /* _TIMER_H */
