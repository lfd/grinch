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

#include <grinch/align.h>
#include <grinch/alloc.h>
#include <grinch/gfp.h>
#include <grinch/limits.h>
#include <grinch/minmax.h>
#include <grinch/panic.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/types.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

/* Let's keep the zero page here, until we need to share it */
static const unsigned long
zero_page[PAGE_SIZE / sizeof(unsigned long)] __aligned(PAGE_SIZE);

bool is_urange(const void *_base, size_t size)
{
	uintptr_t base = (uintptr_t)_base;

	if (base >= USER_START && base < USER_END &&
	    (base + size) <= USER_END)
		return true;

	return false;
}

void *user_to_direct(struct mm *mm, const void __user *s)
{
	paddr_t pa;

	pa = paging_get_phys(mm->page_table, s);
	if (pa == INVALID_PHYS_ADDR)
		return NULL;

	return p2v(pa);
}

static void *user_to_direct_fault(struct task *t, void __user *s)
{
	void *ret;
	int err;

	ret = user_to_direct(&t->process.mm, s);
	if (!ret) {
		err = process_handle_fault(t, s, true);
		if (err)
			return NULL;

		ret = user_to_direct(&t->process.mm, s);
	}

	return ret;
}

/* This routine is only meant for reading from user pages! */
static const void *user_to_direct_null(struct task *t, const void __user *s)
{
	struct vma *vma;
	void *ret;

	ret = user_to_direct(&t->process.mm, s);
	/* If we have a lazy VMA, redirect to the null page. */
	if (!ret) {
		vma = uvma_at(&t->process, s);
		/* No VMA behind from? We're out. */
		if (!vma)
			return NULL;

		/* A non-lazy VMA must not fault */
		if (!(vma->flags & VMA_FLAG_LAZY))
			BUG();

		ret = (void *)zero_page + page_voffset(s);
	}

	return ret;
}

unsigned long umemset(struct task *t, void *dst, int c, size_t n)
{
	unsigned int remaining;
	unsigned long ret;
	size_t written;
	void *direct;

	ret = 0;
	while (n) {
		direct = user_to_direct_fault(t, dst);
		if (!direct)
			break;

		remaining = page_bytes_left(direct);
		written = min(n, remaining);
		memset(direct, c, written);

		ret += written;
		n -= written;
		dst += written;
	}

	return ret;
}

unsigned long copy_from_user(struct task *t, void *to, const void *from,
			     unsigned long n)
{
	unsigned int remaining;
	const void *direct;
	unsigned long sum;
	size_t written;

	sum = 0;
	while (n) {
		direct = user_to_direct_null(t, from);
		if (!direct)
			break;

		remaining = page_bytes_left(direct);
		written = min(n, remaining);
		memcpy(to, direct, written);

		n -= written;
		to += written;
		from += written;
		sum += written;
	}

	return sum;
}

unsigned long copy_to_user(struct task *t, void *d, const void *s, size_t n)
{
	unsigned int remaining;
	unsigned long copied;
	size_t written;
	void *direct;

	copied = 0;
	while (n) {
		direct = user_to_direct_fault(t, d);
		if (!direct)
			return copied;

		remaining = page_bytes_left(direct);
		written = min(remaining, n);
		memcpy(direct, s, written);

		n -= written;
		s += written;
		d += written;
		copied += written;
	}

	return copied;
}

int uptr_from_user(struct task *t, void *dst, const void __user *user)
{
	unsigned long copied;

	copied = copy_from_user(t, dst, user, sizeof(dst));
	if (copied != sizeof(dst))
		return -EINVAL;

	return 0;
}

int uptr_to_user(struct task *t, void __user *dst, void *ptr)
{
	unsigned long copied;

	if (!PTR_IS_ALIGNED(dst, sizeof(dst)))
			return -EINVAL;

	copied = copy_to_user(t, dst, &ptr, sizeof(ptr));
	if (copied != sizeof(dst))
		return -EINVAL;

	return 0;
}

/*
 * Does NOT pad with zeroes.
 * Returns number of copied bytes including trailing zero.
 * Accepts NULL as destination. In that case, it acts like strlen+1.
 */
ssize_t ustrncpy(char *dst, const char *src, unsigned long count)
{
	unsigned int remaining;
	struct task *task;
	char *direct;
	long copied;

	task = current_task();
	if (task->type != GRINCH_PROCESS)
		return -EINVAL;

	copied = 0;
	while (1) {
		direct = user_to_direct(&task->process.mm, src);
		if (!direct)
			return -EFAULT;

		remaining = page_bytes_left(direct);
		while (remaining) {
			if (count == 0)
				return -ERANGE;

			if (dst) {
				*dst = *direct;
				dst++;
			}
			copied++;
			if (*direct == '\0')
				return copied;

			src++;
			direct++;

			count--;
			remaining--;
		}
	}
}

ssize_t ustrlen(const char __user *src)
{
	ssize_t len;

	len = ustrncpy(NULL, src, ULONG_MAX);
	if (len < 0)
		return len;

	return len - 1;
}

int uenv_dup(struct task *t, const char *const __user *_user,
	     struct uenv_array *uenv)
{
	const char __user *const *user;
	unsigned int elements, *cut;
	char __user *ustring;
	size_t length;
	ssize_t ret;
	char *pos;
	int err;

	memset(uenv, 0, sizeof(*uenv));
	if (!_user)
		return 0;

	/* determine length and number of arguments */
	length = 0;
	elements = 0;
	for (user = _user; ; user++) {
		err = uptr_from_user(t, &ustring, user);
		if (err)
			return err;
		if (!ustring)
			break;

		ret = ustrlen(ustring);
		if (ret < 0)
			return ret;
		length += ret + 1;
		elements++;
	}

	if (length > PAGE_SIZE)
		return -E2BIG;

	/* allocate space in kernel memory */
	uenv->string = kmalloc(length);
	if (!uenv->string)
		return -ENOMEM;

	uenv->cuts = kmalloc(elements * sizeof(uenv->cuts));
	if (!uenv->cuts) {
		kfree(uenv->string);
		uenv->string = NULL;
		return -ENOMEM;
	}

	uenv->length = length;
	uenv->elements = elements;

	/* copy over stuff */
	pos = uenv->string;
	cut = uenv->cuts;
	for (user = _user; length; user++) {
		*cut++ = pos - uenv->string;
		err = uptr_from_user(t, &ustring, user);
		if (err)
			return err;
		if (!ustring)
			break;

		ret = ustrncpy(pos, ustring, length);
		if (unlikely(ret < 0))
			return ret;
		pos += ret;
		length -= ret;
	}
	return 0;
}

void uenv_free(struct uenv_array *uenv)
{
	if (uenv->string)
		kfree(uenv->string);
	if (uenv->cuts)
		kfree(uenv->cuts);
	memset(uenv, 0, sizeof(*uenv));
}
