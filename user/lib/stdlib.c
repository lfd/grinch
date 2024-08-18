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

#include <asm-generic/paging.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <grinch/const.h>
#include <grinch/salloc.h>

#define HEAP_SIZE	(32 * KIB)

char **environ;

static struct {
	void *base;
	size_t size;
} heap;

/* Heap management routines */
int heap_init(void)
{
	int err;

	/* errno is already set by (s)brk */
	heap.base = sbrk(0);
	if (heap.base == (void *)-1) {
		heap.base = NULL;
		return -errno;
	}

	err = brk(heap.base + HEAP_SIZE);
	if (err == -1)
		return -errno;

	heap.size = HEAP_SIZE;

	return salloc_init(heap.base, heap.size);
}

static int heap_increase(size_t sz)
{
	int err;

	sz = page_up(sz);
	if (sbrk(sz) == (void *)-1)
		return -errno;

	heap.size += sz;
	err = salloc_increase(heap.base, sz);
	if (err) {
		dprintf(stderr, "salloc_increase: %s\n", salloc_err_str(err));
		exit(err);
	}

	return 0;
}

/* Regular stdlib routines */
char *getenv(const char *name)
{
	char **envp;
	size_t len;

	len = strlen(name);

	for (envp = environ; *envp; envp++)
		if (!strncmp(name, *envp, len))
			goto found;

	return NULL;

found:
	if (*(*envp + len) != '=')
		return NULL;

	return *envp + len + 1;
}

/* FIXME: malloc, realloc and free need later a lock for thread safety. */
void *malloc(size_t size)
{
	size_t increase;
	void *ret;
	int err;

retry:
	err = salloc_alloc(heap.base, size, &ret, &increase);
	if (err == -ENOMEM) {
		err = heap_increase(increase);
		if (err)
			return NULL;

		goto retry;
	}

	if (err) {
		errno = -err;
		return NULL;
	}

	return ret;
}

void free(void *ptr)
{
	int err;

	err = salloc_free(ptr);
	if (err) {
		dprintf(stderr, "fault: %p: %s\n", ptr, salloc_err_str(err));
		exit(err);
	}
}

void *realloc(void *ptr, size_t size)
{
	size_t increase;
	void *new;
	int err;

retry:
	err = salloc_realloc(heap.base, ptr, size, &new, &increase);
	if (err == -ENOMEM) {
		err = heap_increase(increase);
		if (err)
			return NULL;

		goto retry;
	}

	if (!err)
		return new;

	if (err == -ENOMEM)
		return NULL;

	dprintf(stderr, "realloc fault: %p: %s\n", ptr, salloc_err_str(err));
	exit(err);
}
