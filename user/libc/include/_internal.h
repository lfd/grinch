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

/* This header is internal only, and must not be included directly by users */

#ifndef _INTERNAL_H
#define _INTERNAL_H

#include <grinch/kinfo_abi.h>

extern struct __libc __libc;

struct __libc {
	size_t *auxv;
	struct kinfo *kinfo;
};

#endif
