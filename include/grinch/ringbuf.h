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

#ifndef _RINGBUF_H
#define _RINGBUF_H

#include <asm/spinlock.h>

struct ringbuf {
	char *buf;
	unsigned int head;
	unsigned int tail;
	unsigned int size;
};

int ringbuf_init(struct ringbuf *rb, unsigned int size);
void ringbuf_free(struct ringbuf *rb);

/* How much bytes are currently stored in the buffer? Max: size */
static inline unsigned int ringbuf_count(struct ringbuf *rb)
{
	return rb->tail - rb->head;
}

void ringbuf_write(struct ringbuf *rb, char c);
char *ringbuf_read(struct ringbuf *rb, unsigned int *sz);

static inline void ringbuf_consume(struct ringbuf *rb, unsigned int sz)
{
	rb->head += sz;
}

#endif /* _RINGBUF_H */
