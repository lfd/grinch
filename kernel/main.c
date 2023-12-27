/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"main: " x

#include <asm/irq.h>

#include <grinch/alloc.h>
#include <grinch/arch.h>
#include <grinch/boot.h>
#include <grinch/errno.h>
#include <grinch/memtest.h>
#include <grinch/percpu.h>
#include <grinch/kmm.h>
#include <grinch/printk.h>
#include <grinch/task.h>

static const char logo[] =
"\n\n"
"            _            _\n"
"           (_)          | |\n"
"  __ _ _ __ _ _ __   ___| |_\n"
" / _` | '__| | '_ \\ / __| '_ \\\n"
"| (_| | |  | | | | | (__| | | |\n"
" \\__, |_|  |_|_| |_|\\___|_| |_|\n"
"  __/ |\n"
" |___/";

static const char logo_vm[] =
"\n\n"
"            _            _  __  _____    __\n"
"           (_)          | | \\ \\/ /| |\\  /| |\n"
"  __ _ _ __ _ _ __   ___| |_ \\  / | | \\/ | |\n"
" / _` | '__| | '_ \\ / __| '_\\ \\/  |_| \\/ |_|\n"
"| (_| | |  | | | | | (__| | | |\n"
" \\__, |_|  |_|_| |_|\\___|_| |_|\n"
"  __/ |\n"
" |___/";

static const char hello[] =
#ifdef ARCH_RISCV
"       on RISC-V\n"
#endif
"\n      -> Welcome to Grinch " __stringify(GRINCH_VER) " <- \n\n\n";

unsigned int grinch_id;

static int init(void)
{
	struct task *task;
	int err;

	task = process_alloc_new();
	if (IS_ERR(task))
		return PTR_ERR(task);

	err = process_from_fs(task, "initrd:/init.echse");
	if (err) {
		task_destroy(task);
		return err;
	}

	sched_enqueue(task);

	return 0;
}

int cmain(unsigned long boot_cpu, paddr_t __fdt)
{
	int err;

	irq_disable();

	guest_init();

	if (grinch_is_guest)
		_puts(logo_vm);
	else
		_puts(logo);
	_puts(hello);
	printk_init();

	err = kmm_init();
	if (err)
		goto out;

	ps("Activating final paging\n");
	err = paging_init(boot_cpu);
	if (err)
		goto out;

	pr("CPU ID: %lu\n", this_cpu_id());

	err = arch_init(__fdt);
	if (err)
		goto out;

	if (0)
		memtest();

	kheap_stats();

	this_per_cpu()->schedule = true;
	if (1) {
		ps("Initialising userland\n");
		err = init();
		if (err)
			ps("Error initialising userland\n");
		err = 0;
	}

	prepare_user_return();

out:
	pr("End reached: %d\n", err);
	return err;
}
