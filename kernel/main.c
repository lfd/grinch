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

#define dbg_fmt(x)	"main: " x

#include <asm/irq.h>
#include <asm/isa.h>

#include <grinch/alloc.h>
#include <grinch/arch.h>
#include <grinch/boot.h>
#include <grinch/bootparam.h>
#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/memtest.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
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
static unsigned int num_vms;

unsigned int grinch_id;

static void __init memtest_parse(const char *)
{
	do_memtest = true;
}
bootparam(memtest, memtest_parse);

static void __init num_vms_parse(const char *arg)
{
	unsigned long ret;

	ret = strtoul(arg, NULL, 10);
	if (ret > 10) {
		pri("Limiting to 10 VMs\n");
		ret = 10;
	}

	num_vms = ret;
}
bootparam(num_vms, num_vms_parse);

static int __init init(void)
{
	struct task *task;
	int err;

	pri("Initialising userland\n");
	task = process_alloc_new();
	if (IS_ERR(task))
		return PTR_ERR(task);

	err = process_from_fs(task, ISTR("initrd:/init.echse"));
	if (err) {
		task_destroy(task);
		return err;
	}

	sched_enqueue(task);

	return 0;
}

static int __init vm_init(void)
{
	unsigned int vm;
	int err;

	if (!num_vms)
		return 0;

	if (!has_hypervisor())
		return -ENOSYS;

	for (vm = 0; vm < num_vms; vm++) {
		err = vm_create_grinch();
		if (err)
			return err;
	}

	return err;
}

void cmain(unsigned long boot_cpu, paddr_t __fdt)
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
	this_per_cpu()->primary = true;
	spin_init(&this_per_cpu()->remote_call.lock);

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

	err = vm_init();
	if (err)
		goto out;

	err = init();
	if (err)
		goto out;

	err = paging_discard_init();
	if (err)
		goto out;

	sched_all();

	prepare_user_return();

out:
	pr("End reached: %pe\n", ERR_PTR(err));
	if (err)
		arch_shutdown(err);
}
