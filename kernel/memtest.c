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

static int __init memtest_kmem(void)
{
	unsigned long ctr, num_ptrs;
	void *page, **pages;
	unsigned int tmp;
	int err;
	u64 *i;

	num_ptrs = memory_size() / PAGE_SIZE;
	pri("Allocating space for storing %lu pointers\n", num_ptrs);
	pages = kzalloc(num_ptrs * sizeof(void *));
	if (!pages) {
		pri("Not enough space!\n");
		return -ENOMEM;
	}

	pri("Running Memtest...\n");
	for (ctr = 0; ctr < num_ptrs; ctr++) {
		pr_raw_dbg_i("\r#%lu", ctr);
		page = alloc_pages(1);
		if (!page) {
			pr_raw_i(" -> Out of memory");
			goto allocated;
		}

		for (i = page; (void*)i < page + PAGE_SIZE; i++) {
			if (*i) {
				pr_raw_i(" -> Page not zero: %p (phys: 0x%llx) = 0x%llx\n", i, v2p(i), *i);
				break;
			}
		}

		pages[ctr] = page;
	}
	pr_raw_i("\n");
	pri("Too few space for pointers!\n");

allocated:
	pr_raw_i("\n");
	pri("Allocated %lu pages\n", ctr);

	pri("Freeing pages...\n");
	for (tmp = 0; tmp < ctr; tmp++) {
		err = free_pages(pages[tmp], 1);
		if (err) {
			pri("Error: %pe\n", ERR_PTR(err));
			return err;
		}
	}

	kfree(pages);

	return 0;
}

static int __init memtest_kmalloc(void)
{
	unsigned long num_ptrs, i;
	size_t sz = 1000;
	void **ptrs;

	num_ptrs = kheap_size() / sz;
	pri("Allocating space for storing %lu pointers\n", num_ptrs);
	ptrs = kzalloc(num_ptrs * sizeof(*ptrs));
	if (!ptrs)
		return -ENOMEM;

	for (i = 0; i < num_ptrs; i++) {
		pr_raw_dbg_i("A #%lu\r", i);
		ptrs[i] = kmalloc(sz);
		if (!ptrs[i])
			break;
	}
	pr_raw_dbg_i("\n");

	for (i = 0; i < num_ptrs; i++) {
		pr_raw_dbg_i("F #%lu\r", i);
		if (!ptrs[i])
			break;
		kfree(ptrs[i]);
	}
	pr_raw_dbg_i("\n");

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
