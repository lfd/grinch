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

#include <asm/cpu.h>
#include <asm/irq.h>

#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/init.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/timer.h>

#include <grinch/arch/sbi.h>

u32 riscv_timebase_frequency;

static __initdata int _err;

static inline u64 get_time(void)
{
	return csr_read(time);
}

unsigned long arch_timer_ticks_to_time(unsigned long ticks)
{
	return NSEC_PER_SEC * ticks / riscv_timebase_frequency;
}

unsigned long arch_timer_get(void)
{
	return arch_timer_ticks_to_time(get_time());
}

void arch_timer_set(unsigned long ns)
{
	struct sbiret ret;
	u64 then;

	then = (u64)ns * riscv_timebase_frequency / NSEC_PER_SEC;

	// FIXME: implement SSTC
	ret = sbi_set_timer(then);
	if (ret.error)
		panic("SBI Error\n");
}

static void __init arch_timer_cpu_init(void *)
{
	struct sbiret ret;

	timer_disable();
	// FIXME: implement SSTC
	ret = sbi_set_timer(-1);
	if (ret.error) {
		_err = -EINVAL;
		mb();
	}
}

int __init arch_timer_init(void)
{
	int err, nodeoffset;

	pri("Initialising platform timer\n");
	on_each_cpu(arch_timer_cpu_init, NULL);
	if (_err)
		return _err;

	nodeoffset = fdt_path_offset(_fdt, ISTR("/cpus"));
	if (nodeoffset <= 0)
		return -ENOENT;

	err = fdt_read_u32(_fdt, nodeoffset, ISTR("timebase-frequency"),
			   &riscv_timebase_frequency);
	if (err) {
		pri("No valid timebase frequency found\n");
		return err;
	}

	return 0;
}
