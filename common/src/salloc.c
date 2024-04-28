/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/align.h>
#include <grinch/errno.h>
#include <grinch/minmax.h>
#include <grinch/salloc.h>
#include <grinch/string.h>
#include <grinch/types.h>

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

const char *salloc_err_str(int err)
{
	switch (err) {
		case -EINVAL:
			return "corrupted heap structures";

		case -EMSGSIZE:
			return "size mismatch";

		case -EAGAIN:
			return "potential double free";

		case 0:
			return "success";

		default:
			return "Unknown";
	}
}

static int __must_check check_chunk(struct memchunk *chunk)
{
	if (chunk->canary1 != CANARY1 || chunk->canary2 != CANARY2)
		return -EINVAL;
	return 0;
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

static inline struct memchunk *chunk_of(const void *ptr)
{
	return (struct memchunk *)(ptr - sizeof(struct memchunk));
}

int salloc_init(void *base, size_t size)
{
	/* smaller sizes do not make sense at all. */
	if (size < 1024)
		return -ENOMEM;

	set_chunk(base, NULL, size - sizeof(struct memchunk),
		  MEMCHUNK_FLAG_LAST);

	return 0;
}

int salloc_alloc(void *base, size_t size, void **dst)
{
	struct memchunk *this, *other, *tmp;
	unsigned int flags, remaining;
	void *ret;
	int err;

	size = ALIGN(size, 4);
	this = base;

	do {
		err = check_chunk(this);
		if (err)
			return err;

		if (this->flags & MEMCHUNK_FLAG_USED || this->size < size) {
			this = next_chunk(this);
			if (!this)
				return -ENOMEM;
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
					return -EINVAL;

				err = check_chunk(tmp);
				if (err)
					return err;

				tmp->before = other;
			}

			remaining = this->size - size - sizeof(struct memchunk);
			set_chunk(other, this, remaining, flags);
			this->size = size;
		}

		ret = chunk_data(this);
		break;
	} while (true);

	*dst = ret;
	return 0;
}

int salloc_realloc(void *base, void *old, size_t size, void **_new)
{
	struct memchunk *m_old, *m_new;
	void *new;
	int err;

	m_old = chunk_of(old);
	err = check_chunk(m_old);
	if (err)
		return err;

	err = salloc_alloc(base, size, &new);
	if (err)
		return err;

	m_new = chunk_of(new); /* must be valid */
	memcpy(new, old, min(m_old->size, m_new->size));
	*_new = new;

	return 0;
}

int salloc_free(const void *ptr)
{
	struct memchunk *m, *before, *after, *tmp;
	int err;

	m = chunk_of(ptr);
	err = check_chunk(m);
	if (err)
		return err;

	if (!(m->flags & MEMCHUNK_FLAG_USED))
		return -EAGAIN;

	m->flags &= ~MEMCHUNK_FLAG_USED;

	/* Merge before */
	before = m->before;
	after = next_chunk(m);
	if (after) {
		err = check_chunk(after);
		if (err)
			return err;
	}

	if (before) {
		err = check_chunk(before);
		if (err)
			return err;

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
			err = check_chunk(tmp);
			if (err)
				return err;
			tmp->before = m;
		}

		m->size += sizeof(struct memchunk) + after->size;
		m->flags = after->flags; /* Carries over MEMCHUNK_FLAG_LAST */
		memset(after, 0, sizeof(struct memchunk));
	}

	return 0;
}

int salloc_stats(salloc_printer pr, void *base)
{
	size_t used, free, chunks_free, chunks_used;
	struct memchunk *this;
	int err;

	used = free = chunks_free = chunks_used = 0;
	this = base;

	do {
		err = check_chunk(this);
		if (err)
			return err;

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

	pr("Chunks Free: %lu Chunks Used: %lu Free: 0x%lx Used: 0x%lx\n",
	   chunks_free, chunks_used, free, used);

	return 0;
}

int salloc_fsck(salloc_printer pr, void *base, size_t _size)
{
	static unsigned long ctr;
	struct memchunk *this;
	unsigned long size;
	int err;

	pr("salloc_fsck: run %lu\n", ctr++);
	/* Forward run */
	this = base;
	size = 0;
	while (1) {
		err = check_chunk(this);
		if (err)
			return err;
		size += sizeof(*this) + this->size;
		if (this->size == 0)
			pr("salloc_fsck: found chunk with zero size\n");

		if (this->flags & MEMCHUNK_FLAG_LAST)
			break;
		else
			this = next_chunk(this);
	}
	if (size != _size) {
		pr("fsck forward: %s\n", salloc_err_str(-EMSGSIZE));
		return -EMSGSIZE;
	}

	/* Backward run */
	size = 0;
	while (1) {
		err = check_chunk(this);
		if (err)
			return err;
		size += sizeof(*this) + this->size;
		if (this->size == 0)
			pr("Found chunk with zero size\n");

		if (this->before == NULL)
			break;
		else
			this = this->before;
	}
	if (size != _size) {
		pr("fsck backward: %s\n", salloc_err_str(-EMSGSIZE));
		return -EMSGSIZE;
	}
	return 0;
}
