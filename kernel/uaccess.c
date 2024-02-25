/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"uaccess: " x

#include <grinch/gfp.h>
#include <grinch/panic.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/types.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

static inline unsigned int bytes_in_page(const void *p)
{
	return PAGE_SIZE - ((uintptr_t)p & PAGE_OFFS_MASK);
}

void *user_to_direct(struct mm *mm, const void *s)
{
	paddr_t pa;

	pa = paging_get_phys(mm->page_table, s);
	if (pa == INVALID_PHYS_ADDR)
		return NULL;

	return p2v(pa);
}

void umemset(struct mm *mm, void *s, int c, size_t n)
{
	unsigned int remaining;
	size_t written;
	void *direct;

	while (n) {
		direct = user_to_direct(mm, s);
		if (!direct)
			panic("Invalid user address: %p\n", s);

		remaining = bytes_in_page(direct);
		if (n > remaining) {
			memset(direct, c, remaining);
			written = remaining;
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
	unsigned int remaining;
	unsigned long sum;
	size_t written;
	void *direct;

	sum = 0;
	while (n) {
		direct = user_to_direct(mm, from);
		if (!direct)
			break;

		remaining = bytes_in_page(direct);
		if (n > remaining) {
			memcpy(to, direct, remaining);
			written = remaining;
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
	unsigned int remaining;
	size_t written;
	void *direct;

	while (n) {
		direct = user_to_direct(mm, d);
		if (!direct)
			panic("Invalid user address: %p\n", s);

		remaining = bytes_in_page(direct);
		if (n > remaining) {
			memcpy(direct, s, remaining);
			written = remaining;
		} else {
			memcpy(direct, s, n);
			written = n;
		}

		n -= written;
		s += written;
		d += written;
	}
}
