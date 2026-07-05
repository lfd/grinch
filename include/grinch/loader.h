/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef __LOADER_H
#define __LOADER_H

#include <grinch/paging.h>
#include <grinch/string.h>

static inline void *loader_page_zalloc(void **next)
{
	void *tmp = *next;

	memset(tmp, 0, PAGE_SIZE);
	*next += PAGE_SIZE;

	return tmp;
}

#endif /* __LOADER_H */
