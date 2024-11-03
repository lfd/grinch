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

#define dbg_fmt(x)	"timer: " x

#include <asm/irq.h>

#include <grinch/bootparam.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/task.h>
#include <grinch/timer.h>

static unsigned int timer_hz = 50;
static timeu_t wall_base;

static void __init timer_hz_parse(const char *arg)
{
	unsigned long ret;

	ret = strtoul(arg, NULL, 10);
	if (ret > 100000) {
		pri("Invalid timer frequency: %ld\n", ret);
		return;
	}

	timer_hz = ret;
}
bootparam(timer_hz, timer_hz_parse);

void timer_ticks_to_time(timeu_t ticks, struct timespec *ts)
{
	timeu_t ns;

	ns = arch_timer_ticks_to_time(ticks) - wall_base;
	ns_to_ts(ns, ts);
}

void timer_get_wall(struct timespec *ts)
{
	if (!wall_base) {
		*ts = (struct timespec){0};
		return;
	}

	ns_to_ts(timer_get_wall_ns(), ts);
}

timeu_t timer_get_wall_ns(void)
{
	return arch_timer_get() - wall_base;
}

void timer_update(struct task *task)
{
	timeu_t *next;

	next = &this_per_cpu()->timer.next;
	if (task && task->wfe.timer.expiration < *next)
		*next = task->wfe.timer.expiration;

	if (*next != (timeu_t)-1)
		arch_timer_set(*next + wall_base);
	else
		arch_timer_set(-1);
}

void handle_timer(void)
{
	struct per_cpu *tpcpu;
	timeu_t next;

	tpcpu = this_per_cpu();

	tpcpu->schedule = true;
	tpcpu->handle_events = true;

	// FIXME: make me cyclic
	if (timer_hz)
		next = timer_get_wall_ns() + HZ_TO_NS(timer_hz);
	else
		next = -1;
	tpcpu->timer.next = next;
}

static void __init timer_cpu_init(void *)
{
	timeu_t *next;

	next = &this_per_cpu()->timer.next;
	if (timer_hz)
		*next = HZ_TO_NS(timer_hz);
	else
		*next = -1;
	arch_timer_set(*next + wall_base);
	timer_enable();
}

int __init timer_init(void)
{
	struct timespec ts;
	int err;

	err = arch_timer_init();
	if (err)
		return err;

	pri("Timer Frequency: %uHz\n", timer_hz);
	wall_base = arch_timer_get();

	ns_to_ts(wall_base, &ts);
	pri("Wall base: " PR_TS_FMT "\n", PR_TS_PARAMS(&ts));
	on_each_cpu(timer_cpu_init, NULL);

	return err;
}
