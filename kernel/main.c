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

#include <grinch/alloc.h>
#include <grinch/arch.h>
#include <grinch/boot.h>
#include <grinch/bootparam.h>
#include <grinch/console.h>
#include <grinch/cpu.h>
#include <grinch/driver.h>
#include <grinch/fdt.h>
#include <grinch/fs/devfs.h>
#include <grinch/fs/initrd.h>
#include <grinch/fs/vfs.h>
#include <grinch/gcov.h>
#include <grinch/gfp.h>
#include <grinch/header.h>
#include <grinch/memtest.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/platform.h>
#include <grinch/ttp.h>
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
"       on " __stringify(ARCH) "\n"
"\n      -> Welcome to Grinch " VERSION_STRING " <- \n\n\n";

static bool __initdata do_memtest;

unsigned int grinch_id;

static void __init memtest_parse(const char *)
{
	do_memtest = true;
}
bootparam(memtest, memtest_parse);

static char __initdata f_init[32] = "/initrd/bin/init";
static void __init init_parse(const char *arg)
{
	strncpy(f_init, arg, sizeof(f_init) - 1);
	f_init[sizeof(f_init) - 1] = 0;
}
bootparam(init, init_parse);

static int __init init(void)
{
	struct file_handle *fh;
	struct task *task;
	unsigned int i;
	int err;

	pri("Initialising userland\n");
	task = process_alloc_new("init");
	if (IS_ERR(task))
		return PTR_ERR(task);

	err = process_from_path(task, NULL, f_init, NULL, NULL);
	if (err)
		goto exit_out;

	/* stdin */
	fh = &task->process.fds[0];
	fh->flags.may_read = true;
	fh->flags.may_write = false;
	fh->flags.is_kernel = false;
	fh->flags.nonblock = false;

	/* stdout + stderr */
	fh = &task->process.fds[1];
	fh->flags.may_read = false;
	fh->flags.may_write = true;
	fh->flags.is_kernel = false;
	fh->flags.nonblock = false;
	task->process.fds[2].flags = fh->flags;

	for (i = 0; i < 3; i++) {
		fh = &task->process.fds[i];
		fh->fp = file_open_at(NULL, ISTR(DEVICE_NAME("console")));
		if (IS_ERR(fh->fp)) {
			err = PTR_ERR(fh->fp);
			goto exit_out;
		}
	}

	err = process_setcwd(task, ISTR("/"));
	if (err)
		goto exit_out;

	task->state = TASK_RUNNABLE;

	init_task = task;

	sched_enqueue(task);

	return 0;

exit_out:
	task_exit(task, err);
	task_destroy(task);
	return err;
}

void cmain(unsigned long boot_cpu, paddr_t __fdt)
{
	int err;

	irq_disable();

	gcov_init();
	arch_guest_init();

	_puts(ISTR("\n" UNAME_A "\n"));
	if (grinch_is_guest)
		_puts(logo_vm);
	else
		_puts(logo);
	_puts(hello);
	printk_init();
	pr_dbg_i("Compiler CFLAGS:" COMPILE_CFLAGS "\n");

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

	err = phys_mem_init_fdt();
	if (err)
		goto out;

	err = initrd_init();
	if (err == -ENOENT)
		pri("No ramdisk found\n");
	else if (err)
		goto out;

	err = kheap_init();
	if (err)
		goto out;

	err = platform_init();
	if (err)
		goto out;

	err = arch_init();
	if (err)
		goto out;

	err = task_init();
	if (err)
		goto out;

	err = vfs_init();
	if (err)
		goto out;

	if (do_memtest) {
		err = memtest();
		if (err)
			goto out;
	}

	err = driver_init();
	if (err && err != -ENOENT)
		goto out;

	err = console_init();
	if (err) {
		pr_crit_i("Error initialising console: %pe\n", ERR_PTR(err));
		goto out;
	}

	err = init();
	if (err)
		goto out;

	err = ttp_init();
	if (err)
		goto out;

	err = paging_discard_init();
	if (err)
		goto out;

	sched_all();

	prepare_user_return();

out:
	if (err) {
		pr("End reached: %pe\n", ERR_PTR(err));
		arch_shutdown(err);
	}
}

const struct grinch_header __attribute__((section(".header1")))
grinch_header = {
	.signature = GRINCH_SIGNATURE,
	.gcov_info_head = GCOV_HEAD,
};
