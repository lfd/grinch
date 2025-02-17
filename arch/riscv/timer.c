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

#include <grinch/div64.h>
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

timeu_t arch_timer_ticks_to_time(timeu_t ticks)
{
	u64 ret;

	ret = NSEC_PER_SEC * ticks;
	do_div(ret, riscv_timebase_frequency);

	return ret;
}

timeu_t arch_timer_get(void)
{
	return arch_timer_ticks_to_time(get_time());
}

void arch_timer_set(timeu_t ns)
{
	struct sbiret ret;
	timeu_t then;

	then = ns * riscv_timebase_frequency;
	do_div(then, NSEC_PER_SEC);

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
