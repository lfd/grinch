/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _UACCESS_H
#define _UACCESS_H

struct mm;

void *user_to_direct(struct mm *mm, const void *s);

unsigned long umemset(struct mm *mm, void __user *s, int c, size_t n);
unsigned long copy_to_user(struct mm *mm, void __user *d,
			   const void *s, size_t n);
unsigned long copy_from_user(struct mm *mm, void *to, const void __user *from,
			     unsigned long n);

long ustrncpy(char *dst, const char __user *src, long count);
#endif
