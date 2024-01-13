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
static unsigned long long wall_base;
static unsigned long long timer_expected;

static void __init timer_hz_parse(const char *arg)
{
	unsigned long ret;

	ret = strtoul(arg, NULL, 10);
	if (ret > 100000 || !ret) {
		pr("Invalid timer frequency: %ld\n", ret);
		return;
	}

	timer_hz = ret;
}
bootparam(timer_hz, timer_hz_parse);

unsigned long long timer_ticks_to_time(unsigned long long ticks)
{
	return arch_timer_ticks_to_time(ticks) - wall_base;
}

unsigned long long timer_get_wall(void)
{
	if (!wall_base)
		return 0;
	return arch_timer_get() - wall_base;
}

int handle_timer(void)
{
	unsigned long long now;

	now = arch_timer_get();
	timer_expected += HZ_TO_NS(timer_hz);
	if (timer_expected < now)
		pr("Overrun!\n");

	arch_timer_set(timer_expected);
	task_handle_events();

	return 0;
}

int __init timer_init(void)
{
	int err;

	err = arch_timer_init();
	if (err)
		return err;

	pr("Timer Frequency: %uHz\n", timer_hz);
	wall_base = arch_timer_get();
	timer_expected = wall_base + HZ_TO_NS(timer_hz);

	pr("Wall base: " PR_TIME_FMT "\n", PR_TIME_PARAMS(wall_base));
	arch_timer_set(timer_expected);
	timer_enable();

	return err;
}
