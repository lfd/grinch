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

#ifndef _GRINCH_SALLOC_H
#define _GRINCH_SALLOC_H

typedef void (*salloc_printer)(const char *format, ...);

/* Routines */
void salloc_init(void *base, size_t size);
int __must_check salloc_alloc(void *base, size_t size, void **dst);
int __must_check salloc_free(const void *ptr);

/* Utilities */
int __must_check salloc_fsck(salloc_printer pr, void *base, size_t size);
int __must_check salloc_stats(salloc_printer pr, void *base);
const char *salloc_err_str(int err);

#endif /* _GRINCH_SALLOC_H */
