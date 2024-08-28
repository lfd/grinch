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

#define dbg_fmt(x) "alloc: " x

#include <asm/spinlock.h>

#include <grinch/align.h>
#include <grinch/alloc.h>
#include <grinch/bootparam.h>
#include <grinch/errno.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/salloc.h>
#include <grinch/vma.h>

static DEFINE_SPINLOCK(alloc_lock);

static struct vma vma_kheap = {
	.base = (void*)KHEAP_BASE,
	.size = 1 * MIB,
	.flags = VMA_FLAG_RW,
};

static bool do_malloc_fsck;

static void __init malloc_fsck_parse(const char *)
{
	do_malloc_fsck = true;
}
bootparam(malloc_fsck, malloc_fsck_parse);

static void __init kheap_size_parse(const char *arg)
{
	size_t sz;
	int err;

	err = bootparam_parse_size(arg, &sz);
	if (err) {
		pri("Warning: Unable to parse kheap_size=%s\n", arg);
		return;
	}

	if (sz < 4 * KIB) {
		pri("Warning: kheap_size too small\n");
		return;
	}

	if (sz > 1 * GIB) {
		pri("Warning: kheap_size too big\n");
		return;
	}

	vma_kheap.size = sz;
}
bootparam(kheap_size, kheap_size_parse);

static bool is_kheap(const void *ptr)
{
	if (ptr >= vma_kheap.base && ptr < vma_kheap.base + vma_kheap.size)
		return true;
	return false;
}

static void __printf(1, 2) kheap_pr(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, dbg_fmt(""), ap);
	va_end(ap);
}

void kheap_stats(void)
{
	int err;

	spin_lock(&alloc_lock);
	err = salloc_stats(kheap_pr, vma_kheap.base);
	spin_unlock(&alloc_lock);

	if (err)
		panic("salloc_stats: %s\n", salloc_err_str(err));
}

void *kmalloc(size_t size)
{
	void *ret;
	int err;

	if (do_malloc_fsck) {
		err = salloc_fsck(kheap_pr, vma_kheap.base, vma_kheap.size);
		if (err)
			panic("salloc_fsck failed: %s\n", salloc_err_str(err));
	}

	spin_lock(&alloc_lock);
	err = salloc_alloc(vma_kheap.base, size, &ret, NULL);
	spin_unlock(&alloc_lock);

	if (err == -ENOMEM)
		return NULL;

	if (err)
		panic("salloc_alloc failed: %s\n", salloc_err_str(err));

	return ret;
}

void kfree(const void *ptr)
{
	int err;

	if (do_malloc_fsck) {
		err = salloc_fsck(kheap_pr, vma_kheap.base, vma_kheap.size);
		if (err)
			panic("salloc_fsck failed: %s\n", salloc_err_str(err));
	}

	/* Do nothing on NULL ptr free */
	if (ptr == NULL)
		return;

	if (!is_kheap(ptr))
		panic("Invalid free on kheap. Out of range: %p\n", ptr);

	spin_lock(&alloc_lock);
	err = salloc_free(ptr);
	spin_unlock(&alloc_lock);

	if (err)
		panic("kfree on %p: %s\n", ptr, salloc_err_str(err));
}

int __init kheap_init(void)
{
	int err;

	pri("Kernel Heap base: %p, size: 0x%lx\n",
	    vma_kheap.base, vma_kheap.size);

	err = kvma_create(&vma_kheap);
	if (err)
		return err;

	err = salloc_init(vma_kheap.base, vma_kheap.size);

	return err;
}

size_t kheap_size(void)
{
	return vma_kheap.size;
}
