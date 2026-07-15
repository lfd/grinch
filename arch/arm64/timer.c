/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
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
#include <grinch/init.h>
#include <grinch/arch.h>
#include <grinch/irqchip.h>
#include <grinch/printk.h>
#include <grinch/timer.h>

/* Virtual timer PPI in GIC-v2 */
#define TIMER_IRQ	27

static u64 timebase_frequency;

timeu_t arch_timer_ticks_to_time(timeu_t ticks)
{
	return NSEC_PER_SEC * ticks / timebase_frequency;
}

timeu_t arch_timer_get(void)
{
	return arch_timer_ticks_to_time(timer_get_ticks());
}

void arch_timer_set(timeu_t ns)
{
	u64 ticks;

	if (ns == (timeu_t)-1) {
		/* Disable the timer entirely */
		arm_write_sysreg(CNTV_CTL_EL0, 0);
		return;
	}

	/*
	 * Use CNTV_CVAL_EL0 (absolute compare value), not CNTV_TVAL_EL0
	 * (relative countdown). Callers pass an absolute time in ns
	 * (wall_base + offset), which we convert to an absolute tick count.
	 */
	ticks = ns * timebase_frequency / NSEC_PER_SEC;
	arm_write_sysreg(CNTV_CVAL_EL0, ticks);
	arm_write_sysreg(CNTV_CTL_EL0, 1);
}

static int timer_arm64_handler(void *unused)
{
	handle_timer();
	return 0;
}

int __init arch_timer_init(void)
{
	int err;

	arm_read_sysreg(CNTFRQ_EL0, timebase_frequency);
	pri("Timebase frequency: %llx\n", timebase_frequency);

	err = irq_register_handler(TIMER_IRQ, timer_arm64_handler, NULL);
	if (err)
		return err;

	/*
	 * The GIC probes via irqchip_init() before arch_init(), so the
	 * PPI can be enabled right here.
	 *
	 * For SMP: each CPU must enable its own banked copy of the PPI
	 * from its own context (GICD_ISENABLER is CPU-banked for PPIs
	 * 16-31).
	 */
	return irqchip_enable_irq(TIMER_IRQ);
}
