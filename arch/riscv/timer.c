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

#define dbg_fmt(x)	"timer: " x

#include <asm/irq.h>

#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/init.h>
#include <grinch/printk.h>
#include <grinch/timer.h>

#include <grinch/arch/sbi.h>

static u32 timebase_frequency;

static inline u64 get_time(void)
{
	return csr_read(time);
}

int arch_handle_timer(void)
{
	int err;

	err = handle_timer();

	return err;
}

unsigned long long arch_timer_ticks_to_time(unsigned long long ticks)
{
	return NS * ticks / timebase_frequency;
}

unsigned long long arch_timer_get(void)
{
	return arch_timer_ticks_to_time(get_time());
}

void arch_timer_set(unsigned long long ns)
{
	struct sbiret ret;
	u64 then;

	then = (u64)ns * timebase_frequency / NS;

	// FIXME: implement SSTC
	ret = sbi_set_timer(then);
	if (ret.error)
		panic("SBI Error\n");
}

int __init arch_timer_init(void)
{
	int err, nodeoffset;
	struct sbiret ret;

	pri("Initialising platform timer\n");
	timer_disable();

	// FIXME: implement SSTC
	ret = sbi_set_timer(-1);
	if (ret.error)
		return ret.error;

	nodeoffset = fdt_path_offset(_fdt, "/cpus");
	if (nodeoffset <= 0)
		return -ENOENT;

	err = fdt_read_u32(_fdt, nodeoffset, "timebase-frequency",
			   &timebase_frequency);
	if (err) {
		psi("No valid timebase frequency found\n");
		return err;
	}

	return 0;
}
