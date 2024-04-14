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

/* Write to user */
unsigned long umemset(struct task *t, void __user *dst, int c, size_t n);
unsigned long copy_to_user(struct task *t, void __user *d,
			   const void *s, size_t n);

/* Read from user */
unsigned long copy_from_user(struct task *t, void *to, const void __user *from,
			     unsigned long n);

/** Utilities **/

/* copies sizeof(void *) bytes behind *user from userspace to kernel space */
int uptr_from_user(struct task *t, void *dst, const void __user *user);
int uptr_to_user(struct task *t, void __user *dst, void *ptr);

/* Only works on current_task() */
ssize_t ustrncpy(char *dst, const char __user *src, unsigned long count);
ssize_t ustrlen(const char __user *src);

#endif
