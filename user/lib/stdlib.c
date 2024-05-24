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

void *malloc(size_t size)
{
	void *ret;
	int err;

	err = salloc_alloc(heap.base, size, &ret);
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
	void *new;
	int err;

	err = salloc_realloc(heap.base, ptr, size, &new);
	if (!err)
		return new;

	if (err == -ENOMEM)
		return NULL;

	dprintf(stderr, "realloc fault: %p: %s\n", ptr, salloc_err_str(err));
	exit(err);
}
