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

#ifndef _UACCESS_H
#define _UACCESS_H

void umemset(struct mm *mm, void *s, int c, size_t n);
void copy_to_user(struct mm *mm, void *d, const void *s, size_t n);
unsigned long copy_from_user(struct mm *mm, void *to, const void *from,
			     unsigned long n);

#endif
