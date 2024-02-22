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

#define dbg_fmt(x)	"memtest: " x

#include <grinch/alloc.h>
#include <grinch/gfp.h>
#include <grinch/printk.h>
#include <grinch/memtest.h>

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
		page = alloc_pages(1);
		if (!page) {
			pr("Err: %ld\n", PTR_ERR(page));
			break;
		}
		pr("Allocated %p -> 0x%llx\n", page, v2p(page));
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
		err = free_pages(pages[tmp], 1);
		if (err) {
			pr("Error: %pe\n", ERR_PTR(err));
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

void memtest(void)
{
	memtest_kmem();
	memtest_kmalloc();
}
