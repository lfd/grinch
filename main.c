/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"main: " x

#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/percpu.h>
#include <grinch/mm.h>
#include <grinch/plic.h>
#include <grinch/printk.h>
#include <grinch/irq.h>
#include <grinch/serial.h>
#include <grinch/sbi.h>
#include "config.h"

#define LOOPS 0xfffffffUL

void cmain(paddr_t fdt);

unsigned long hz;

static const char logo[] =
"\n\n"
"            _            _\n"
"           (_)          | |\n"
"  __ _ _ __ _ _ __   ___| |_\n"
" / _` | '__| | '_ \\ / __| '_ \\\n"
"| (_| | |  | | | | | (__| | | |\n"
" \\__, |_|  |_|_| |_|\\___|_| |_|\n"
"  __/ |\n"
" |___/\n"
"      -> Welcome to Grinch " __stringify(GRINCH_VER) " <- \n\n\n";

static unsigned long timer_value;

int handle_timer(void)
{
	unsigned long now = get_time();
	unsigned long delta;
	unsigned long delta_us;
	unsigned long delta_cyc;

	delta = now - timer_value;
	delta_us = timer_to_us(delta);
	delta_cyc = us_to_cpu_cycles(delta_us);
	pr("Jtr: %luus (%lu cycles)\n", delta_us, delta_cyc);
	timer_value += TIMEBASE_DELTA;
	sbi_set_timer(timer_value);
	return 0;
}

void cmain(paddr_t __fdt)
{
	int err;

	_puts(logo);

	pr("Hartid: %lu\n", this_cpu_id());

	err = mm_init();
	if (err)
		goto out;

	ps("Activating final paging\n");
	err = paging_init();
	if (err)
		goto out;

	err = fdt_init(__fdt);
	if (err)
		goto out;

	err = mm_init_late();
	if (err)
		goto out;

	err = platform_init();
	if (err)
		goto out;

	err = sbi_init();
	if (err)
		goto out;

	ps("Disabling IRQs\n");
	irq_disable();

	/* Initialise external interrupts */
	ps("Initialising PLIC...\n");
	err = plic_init();
	if (err)
		goto out;

	/* disable external IRQs */
	ext_disable();

	ps("Initialising Serial...\n");
	err = serial_init();
	if (err)
		goto out;
	ps("Switched over from SBI to UART\n");

	ps("Measuring core frequency...\n");
	unsigned long t;
	t = get_time();
	asm volatile(
		"li		t0, " __stringify(LOOPS) "\n"
		".p2align 3\n"
		"1: addi	t0, t0, -1\n"
		"bnez		t0, 1b\n" ::: "t0", "memory");
	t = get_time() - t;
	/* calculate instructions that were executed during that time */
	hz = (1 * LOOPS * US_PER_SEC) / timer_to_us(t);
	pr("Took %lluus for %lu loops -> %luHz (%luMHz) core speed\n", timer_to_us(t), LOOPS, hz, hz / (1000*1000));

	/* Boot secondary CPUs */
	ps("Booting secondary CPUs\n");
	err = smp_init();
	if (err)
		goto out;

#if defined(MEAS_IPI)
	pr("%lu: Waiting for IPIs...\n", this_cpu_id());
	for (;;) {
		while (!(csr_read(sip) & IE_SIE));
		csr_clear(sip, IE_SIE);

		/* Danger: We don't check errors here */
		sbi_send_ipi((1UL << eval_ipi_target), 0);
	}
#elif defined(MEAS_TMR)
	sbi_set_timer(-1);
	ps("Starting Timer measurement\n");
	timer_value = TIMEBASE_DELTA + get_time();
	timer_enable();
	irq_enable();
	sbi_set_timer(timer_value);
#elif defined(MEAS_PLIC)
	ext_enable();
	irq_enable();
#endif

out:
	pr("End reached: %d\n", err);
}
