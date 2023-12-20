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
#include <grinch/percpu.h>
#include <grinch/kmm.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/vma.h>
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
" |___/"
#ifdef ARCH_RISCV
"       on RISC-V\n"
#endif
"\n      -> Welcome to Grinch " __stringify(GRINCH_VER) " <- \n\n\n";

unsigned int grinch_id;

#undef dbg_fmt
#define dbg_fmt(x)	"memtest: " x

#define PTRS	1024
static void memtest_kmem(void)
{
	unsigned int ctr, tmp;
	void *page;
	int err;
	u64 *i;

	void **pages = kzalloc(PTRS * sizeof(void *));
	if (!pages) {
		pr("No memory\n");
		return;
	}

	pr("Running Memtest...\n");
	for (ctr = 0; ctr < PTRS; ctr++) {
		page = kmm_page_alloc(1);
		if (!page) {
			pr("Err: %ld\n", PTR_ERR(page));
			break;
		}
		pr("Allocated %p -> 0x%llx\n", page, kmm_v2p(page));
		for (i = page; (void*)i < page + PAGE_SIZE; i++) {
			if (*i) {
				pr("  -> Page not zero: %p = 0x%llx\n", i, *i);
				break;
			}
		}

		pages[ctr] = page;
	}
	pr("Allocated %u pages\n", ctr);

	for (tmp = 0; tmp < ctr; tmp++) {
		pr("Freeing %p\n", pages[tmp]);
		err = kmm_page_free(pages[tmp], 1);
		if (err) {
			pr("Err: %d\n", err);
			break;
		}
	}
	pr("Freed %u pages\n", ctr);
	pr("Success.\n");
}

static void memtest_kmalloc(void)
{
	size_t sz = 5;
	unsigned int i;
	void **ptrs;

	ptrs = kzalloc(PTRS * sizeof(*ptrs));
	if (!ptrs) {
		pr("No memory :-(\n");
		return;
	}
	for (i = 0; i < PTRS; i++) {
		pr("i %u, sz: %lu\n", i, sz);
		ptrs[i] = kmalloc(sz);
		if (!ptrs[i]) {
			pr("Malloc error!\n");
			break;
		}
		sz += 12;
	}

	for (i = 0; i < PTRS; i++) {
		pr("i %u, ptr: %p\n", i, ptrs[i]);
		if (!ptrs[i])
			break;
		kfree(ptrs[i]);
	}
	kfree(ptrs);
}

static void memtest(void)
{
	memtest_kmem();
	memtest_kmalloc();
}

#undef dbg_fmt
#define dbg_fmt(x)	"main: " x

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

	_puts(logo);
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

	if (1) {
		err = init();
		if (err)
			ps("Error initialising userland\n");
		err = 0;
	}

	schedule();
	arch_task_restore();

	if (!current_task())
		panic("Nothing to schedule!\n");

out:
	pr("End reached: %d\n", err);
	return err;
}
