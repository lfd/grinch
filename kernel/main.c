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
#include <grinch/bootparam.h>
#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/memtest.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/version.h>

static const char __initconst logo[] =
"\n\n"
"            _            _\n"
"           (_)          | |\n"
"  __ _ _ __ _ _ __   ___| |_\n"
" / _` | '__| | '_ \\ / __| '_ \\\n"
"| (_| | |  | | | | | (__| | | |\n"
" \\__, |_|  |_|_| |_|\\___|_| |_|\n"
"  __/ |\n"
" |___/";

static const char __initconst logo_vm[] =
"\n\n"
"            _            _  __  _____    __\n"
"           (_)          | | \\ \\/ /| |\\  /| |\n"
"  __ _ _ __ _ _ __   ___| |_ \\  / | | \\/ | |\n"
" / _` | '__| | '_ \\ / __| '_\\ \\/  |_| \\/ |_|\n"
"| (_| | |  | | | | | (__| | | |\n"
" \\__, |_|  |_|_| |_|\\___|_| |_|\n"
"  __/ |\n"
" |___/";

static const char __initconst hello[] =
#ifdef ARCH_RISCV
"       on RISC-V\n"
#endif
"\n      -> Welcome to Grinch " VERSION_STRING " <- \n\n\n";

static bool do_memtest;

unsigned int grinch_id;

static void __init memtest_parse(const char *)
{
	do_memtest = true;
}
bootparam(memtest, memtest_parse);

static int __init init(void)
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

	_puts(ISTR("\n" UNAME_A "\n"));
	if (grinch_is_guest)
		_puts(logo_vm);
	else
		_puts(logo);
	_puts(hello);
	printk_init();
	pdi("Compiler CFLAGS:" COMPILE_CFLAGS "\n");

	err = kernel_mem_init();
	if (err)
		goto out;

	pri("Activating final paging\n");
	err = paging_init(boot_cpu);
	if (err)
		goto out;

	pri("CPU ID: %lu\n", this_cpu_id());

	err = fdt_init(__fdt);
	if (err)
		goto out;

	err = bootparam_init();
	if (err)
		goto out;

	err = arch_init();
	if (err)
		goto out;

	err = task_init();
	if (err)
		goto out;

	if (do_memtest)
		memtest();

	kheap_stats();

	this_per_cpu()->schedule = true;
	if (1) {
		psi("Initialising userland\n");
		err = init();
		if (err)
			psi("Error initialising userland\n");
		err = 0;
	}

	err = paging_discard_init();
	if (err)
		goto out;

	prepare_user_return();
out:
	pr("End reached: %d\n", err);
	return err;
}
