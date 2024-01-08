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

#define dbg_fmt(x) "alloc: " x

#include <asm_generic/grinch_layout.h>
#include <asm/spinlock.h>

#include <grinch/alloc.h>
#include <grinch/bootparam.h>
#include <grinch/errno.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/vma.h>

#define CANARY1	0xdeadbeef
#define CANARY2 0xf00bf00b

#define MEMCHUNK_FLAG_LAST	0x1
#define MEMCHUNK_FLAG_USED	0x2

struct memchunk {
	unsigned int canary1;
	struct memchunk *before;
	size_t size;
	unsigned int flags;
	unsigned int canary2;
};

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
		pr("Warning: Unable to parse kheap_size=%s\n", arg);
		return;
	}

	if (sz < 4 * KIB) {
		pr("Warning: kheap_size too small\n");
		return;
	}

	vma_kheap.size = sz;
}
bootparam(kheap_size, kheap_size_parse);

#define first_chunk	((struct memchunk *)(vma_kheap.base))

static void check_chunk(struct memchunk *chunk)
{
	if (chunk->canary1 != CANARY1 || chunk->canary2 != CANARY2)
		panic("Corrupted heap structures!\n");
}

static bool is_kheap(const void *ptr)
{
	if (ptr >= vma_kheap.base && ptr < vma_kheap.base + vma_kheap.size)
		return true;
	return false;
}

static void set_chunk(struct memchunk *m, struct memchunk *before,
		      size_t size, unsigned int flags)
{
	m->canary1 = CANARY1;
	m->size = size;
	m->flags = flags;
	m->before = before;
	m->canary2 = CANARY2;
}

static inline struct memchunk *next_chunk(struct memchunk *this)
{
	if (this->flags & MEMCHUNK_FLAG_LAST)
		return NULL;

	return (void *)this + sizeof(struct memchunk) + this->size;
}

static inline void *chunk_data(struct memchunk *m)
{
	return (void*)m + sizeof(struct memchunk);
}

static inline struct memchunk *chunk_of(void *ptr)
{
	return (struct memchunk *)(ptr - sizeof(struct memchunk));
}

static void malloc_fsck(void)
{
	unsigned long size;
	struct memchunk *this;
	static unsigned long ctr;

	pr("fsck run %lu\n", ctr++);
	/* Forward run */
	this = first_chunk;
	size = 0;
	while (1) {
		check_chunk(this);
		size += sizeof(*this) + this->size;
		if (this->size == 0)
			pr("Found chunk with zero size\n");

		if (this->flags & MEMCHUNK_FLAG_LAST)
			break;
		else
			this = next_chunk(this);
	}
	if (size != vma_kheap.size)
		panic("fsck forw failed!\n");

	/* Backward run */
	size = 0;
	while (1) {
		check_chunk(this);
		size += sizeof(*this) + this->size;
		if (this->size == 0)
			pr("Found chunk with zero size\n");

		if (this->before == NULL)
			break;
		else
			this = this->before;
	}
	if (size != vma_kheap.size)
		panic("fsck back failed\n");
}

void *kmalloc(size_t size)
{
	struct memchunk *this, *other, *tmp;
	unsigned int flags, remaining;
	void *ret;

	if (do_malloc_fsck)
		malloc_fsck();

	/* align size to multiples of 4 */
	size = (size + 3) & ~0x3;

	spin_lock(&alloc_lock);
	this = first_chunk;
	do {
		check_chunk(this);
		if (this->flags & MEMCHUNK_FLAG_USED || this->size < size) {
			this = next_chunk(this);
			if (!this) {
				ret = NULL;
				goto out;
			}
			continue;
		}

		this->flags |= MEMCHUNK_FLAG_USED;
		if (this->size > size + sizeof(struct memchunk)) { /* Split */
			other = chunk_data(this) + size;
			if (this->flags & MEMCHUNK_FLAG_LAST) {
				flags = MEMCHUNK_FLAG_LAST;
				this->flags &= ~MEMCHUNK_FLAG_LAST;
			} else {
				flags = 0;
				tmp = next_chunk(this);
				if (!tmp)
					panic("must not happen\n");
				check_chunk(tmp);
				tmp->before = other;
			}

			remaining = this->size - size - sizeof(struct memchunk);
			set_chunk(other, this, remaining, flags);
			this->size = size;
		}

		ret = chunk_data(this);
		break;
	} while (true);

out:
	spin_unlock(&alloc_lock);
	return ret;
}

void kfree(void *ptr)
{
	struct memchunk *m, *before, *after, *tmp;

	if (do_malloc_fsck)
		malloc_fsck();

	/* Do nothing on NULL ptr free */
	if (ptr == NULL)
		return;

	if (!is_kheap(ptr))
		panic("Invalid free on kheap. Out of range: %p\n", ptr);

	m = chunk_of(ptr);
	spin_lock(&alloc_lock);
	check_chunk(m);

	if (!(m->flags & MEMCHUNK_FLAG_USED))
		panic("Potential Double free on %p\n", ptr);

	m->flags &= ~MEMCHUNK_FLAG_USED;

	/* Merge before */
	before = m->before;
	after = next_chunk(m);
	if (after)
		check_chunk(after);

	if (before) {
		check_chunk(before);
		if (!(before->flags & MEMCHUNK_FLAG_USED)) {
			before->size += sizeof(struct memchunk) + m->size;
			if (after)
				after->before = before;
			memset(m, 0, sizeof(struct memchunk));
			m = before;
		}
	}

	/* Merge after */
	if (after && !(after->flags & MEMCHUNK_FLAG_USED)) {
		tmp = next_chunk(after);
		if (tmp) {
			check_chunk(tmp);
			tmp->before = m;
		}

		m->size += sizeof(struct memchunk) + after->size;
		m->flags = after->flags; /* Carries over MEMCHUNK_FLAG_LAST */
		memset(after, 0, sizeof(struct memchunk));
	}

	spin_unlock(&alloc_lock);
}

void kheap_stats(void)
{
	size_t used, free, chunks_free, chunks_used;
	struct memchunk *this;

	used = free = chunks_free = chunks_used = 0;
	this = first_chunk;
	
	spin_lock(&alloc_lock);
	do {
		check_chunk(this);
		if (this->flags & MEMCHUNK_FLAG_USED) {
			used += this->size;
			chunks_used++;
			pr("Used chunk: %p\n", (void *)this + sizeof(struct memchunk));
		} else {
			free += this->size;
			chunks_free++;
		}

		if (this->flags & MEMCHUNK_FLAG_LAST)
			break;
		this = next_chunk(this);
	} while(1);
	spin_unlock(&alloc_lock);

	pr("Chunks Free: %lu Chunks Used: %lu Free: 0x%lx Used: 0x%lx\n",
	   chunks_free, chunks_used, free, used);
}

int __init kheap_init(void)
{
	int err;

	pri("Kernel Heap base: %p, size: 0x%lx\n",
	    vma_kheap.base, vma_kheap.size);

	err = kvma_create(&vma_kheap);
	if (err)
		return err;

	set_chunk(first_chunk, NULL, vma_kheap.size - sizeof(struct memchunk),
		  MEMCHUNK_FLAG_LAST);

	return 0;
}
