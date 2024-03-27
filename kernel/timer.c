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

#include <string.h>

#include <grinch/bootparam.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/timer.h>

static unsigned int timer_hz = 50;
static unsigned long wall_base;

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

unsigned long timer_ticks_to_time(unsigned long ticks)
{
	return arch_timer_ticks_to_time(ticks) - wall_base;
}

unsigned long timer_get_wall(void)
{
	if (!wall_base)
		return 0;
	return arch_timer_get() - wall_base;
}

void timer_update(struct task *task)
{
	unsigned long *next;

	next = &this_per_cpu()->timer.next;
	if (task && task->wfe.timer.expiration < *next)
		*next = task->wfe.timer.expiration;

	if (*next != (unsigned long)-1)
		arch_timer_set(*next + wall_base);
	else
		arch_timer_set(-1);
}

void handle_timer(void)
{
	struct per_cpu *tpcpu;
	unsigned long next;

	tpcpu = this_per_cpu();

	tpcpu->schedule = true;
	tpcpu->handle_events = true;

	// FIXME: make me cyclic
	if (timer_hz)
		next = timer_get_wall() + HZ_TO_NS(timer_hz);
	else
		next = -1;
	tpcpu->timer.next = next;
}

static void __init timer_cpu_init(void *)
{
	unsigned long *next;

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
	int err;

	err = arch_timer_init();
	if (err)
		return err;

	pri("Timer Frequency: %uHz\n", timer_hz);
	wall_base = arch_timer_get();

	pri("Wall base: " PR_TIME_FMT "\n", PR_TIME_PARAMS(wall_base));
	on_each_cpu(timer_cpu_init, NULL);

	return err;
}
