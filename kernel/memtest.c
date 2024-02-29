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

#define dbg_fmt(x)	"memtest: " x

#include <grinch/alloc.h>
#include <grinch/gfp.h>
#include <grinch/printk.h>
#include <grinch/memtest.h>

#define PTRS	1024

static int __init memtest_kmem(void)
{
	unsigned int ctr, tmp;
	void *page, **pages;
	int err;
	u64 *i;

	pages = kzalloc(PTRS * sizeof(void *));
	if (!pages)
		return -ENOMEM;

	pri("Running Memtest...\n");
	for (ctr = 0; ctr < PTRS; ctr++) {
		page = alloc_pages(1);
		if (!page) {
			pri("Out of memory in run %u\n", ctr);
			break;
		}

		pri("Allocated %p -> 0x%llx\n", page, v2p(page));
		for (i = page; (void*)i < page + PAGE_SIZE; i++) {
			if (*i) {
				pri("  -> Page not zero: %p = 0x%llx\n", i, *i);
				break;
			}
		}

		pages[ctr] = page;
	}
	pri("Allocated %u pages\n", ctr);

	for (tmp = 0; tmp < ctr; tmp++) {
		pri("Freeing %p\n", pages[tmp]);
		err = free_pages(pages[tmp], 1);
		if (err) {
			pri("Error: %pe\n", ERR_PTR(err));
			return err;
		}
	}

	return 0;
}

static int __init memtest_kmalloc(void)
{
	unsigned int i;
	size_t sz = 5;
	void **ptrs;

	ptrs = kzalloc(PTRS * sizeof(*ptrs));
	if (!ptrs)
		return -ENOMEM;

	for (i = 0; i < PTRS; i++) {
		pri("i %u, sz: %lu\n", i, sz);
		ptrs[i] = kmalloc(sz);
		if (!ptrs[i]) {
			pr("Out of memory in run %u!\n", i);
			break;
		}
		sz += 12;
	}

	for (i = 0; i < PTRS; i++) {
		pri("i %u, ptr: %p\n", i, ptrs[i]);
		if (!ptrs[i])
			break;
		kfree(ptrs[i]);
	}
	kfree(ptrs);

	return 0;
}

int __init memtest(void)
{
	int err;

	err = memtest_kmem();
	if (err)
		return err;

	err = memtest_kmalloc();
	if (err)
		return err;

	pri("Memtest run was successful.\n");
	return 0;
}
