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

#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/minmax.h>
#include <grinch/ringbuf.h>

int ringbuf_init(struct ringbuf *rb, unsigned int size)
{
	if (!size)
		return -ERANGE;

	if (size & (size - 1))
		return -ERANGE;

	rb->buf = kzalloc(size);
	if (!rb->buf)
		return -ENOMEM;

	rb->size = size;
	rb->head = 0;
	rb->tail = 0;

	return 0;
}

void ringbuf_free(struct ringbuf *rb)
{
	kfree(rb->buf);
}

void ringbuf_write(struct ringbuf *rb, char c)
{
	unsigned int pos;

	pos = rb->tail % rb->size;

	rb->buf[pos] = c;
	rb->tail++;

	/* Did we loose a byte? */
	if (rb->tail - rb->head > rb->size)
		rb->head++;
}

char *ringbuf_read(struct ringbuf *rb, unsigned int *sz)
{
	unsigned int pos;
	unsigned int cnt;

	cnt = ringbuf_count(rb);
	if (cnt == 0) {
		*sz = 0;
		return NULL;
	}

	pos = rb->head % rb->size;
	*sz = min(cnt, rb->size - pos);

	return &rb->buf[pos];
}
