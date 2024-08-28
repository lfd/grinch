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

/*
 * Routines.
 *
 * If the argument increase is set and salloc_{re}alloc return ENOMEM, then
 * increase contains the number of bytes the underlying structure has to be
 * extended.
 */
int __must_check salloc_init(void *base, size_t size);
int __must_check salloc_alloc(void *base, size_t size, void **dst,
			      size_t *increase);
int __must_check salloc_free(const void *ptr);
int __must_check salloc_realloc(void *base, void *old, size_t size, void **new,
				size_t *increase);

int __must_check salloc_increase(void *base, size_t size);

/* Utilities */
int __must_check salloc_fsck(salloc_printer pr, void *base, size_t size);
int __must_check salloc_stats(salloc_printer pr, void *base);
const char *salloc_err_str(int err);

#endif /* _GRINCH_SALLOC_H */
