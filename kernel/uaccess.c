/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"uaccess: " x

#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/types.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>
#include <grinch/pmm.h>

void *user_to_direct(struct mm *mm, const void *s)
{
	paddr_t pa;

	pa = paging_get_phys(mm->page_table, s);
	if (pa == INVALID_PHYS_ADDR)
		return NULL;

	return pmm_to_virt(pa);
}

void umemset(struct mm *mm, void *s, int c, size_t n)
{
	void *direct;
	size_t written;

	unsigned int remaining_in_page;

	while (n) {
		direct = user_to_direct(mm, s);
		if (!direct)
			panic("Invalid user address: %p\n", s);

		remaining_in_page = PAGE_SIZE - ((uintptr_t)direct & PAGE_OFFS_MASK);

		if (n > remaining_in_page) {
			memset(direct, c, remaining_in_page);
			written = remaining_in_page;
		} else {
			memset(direct, c, n);
			written = n;
		}

		n -= written;
		s += written;
	}
}

unsigned long copy_from_user(struct mm *mm, void *to, const void *from,
			     unsigned long n)
{
	void *direct;
	size_t written;
	unsigned long sum;
	unsigned int remaining_in_page;

	sum = 0;
	while (n) {
		direct = user_to_direct(mm, from);
		if (!direct)
			break;

		remaining_in_page = PAGE_SIZE - ((uintptr_t)direct & PAGE_OFFS_MASK);

		if (n > remaining_in_page) {
			memcpy(to, direct, remaining_in_page);
			written = remaining_in_page;
		} else {
			memcpy(to, direct, n);
			written = n;
		}

		n -= written;
		to += written;
		from += written;
		sum += written;
	}

	return sum;
}

void copy_to_user(struct mm *mm, void *d, const void *s, size_t n)
{
	void *direct;
	size_t written;

	unsigned int remaining_in_page;

	while (n) {
		direct = user_to_direct(mm, d);
		if (!direct)
			panic("Invalid user address: %p\n", s);

		remaining_in_page = PAGE_SIZE - ((uintptr_t)direct & PAGE_OFFS_MASK);

		if (n > remaining_in_page) {
			memcpy(direct, s, remaining_in_page);
			written = remaining_in_page;
		} else {
			memcpy(direct, s, n);
			written = n;
		}

		n -= written;
		s += written;
		d += written;
	}
}
