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

/* Utilities */

/* Check if a given memory range is fully in userspace */
bool is_urange(const void *_base, size_t size);

/* Check if an address is a userspace address */
static inline bool is_uaddr(const void *addr)
{
	return is_urange(addr, 0);
}

/* Write to user */
unsigned long umemset(struct task *t, void __user *dst, int c, size_t n);
unsigned long copy_to_user(struct task *t, void __user *d,
			   const void *s, size_t n);

/* Read from user */
unsigned long copy_from_user(struct task *t, void *to, const void __user *from,
			     unsigned long n);

/** Utilities **/
void *user_to_direct(struct mm *mm, const void __user *uptr);

/* copies sizeof(void *) bytes behind *user from userspace to kernel space */
int uptr_from_user(struct task *t, void *dst, const void __user *user);
int uptr_to_user(struct task *t, void __user *dst, void *ptr);

/* userspace string operation - Only works on current_task() */
ssize_t ustrncpy(char *dst, const char __user *src, unsigned long count);
ssize_t ustrlen(const char __user *src);

/* User environment array copy in kernel-space, such as argv or env */
struct uenv_array {
	char *string;
	size_t length;
	unsigned int *cuts;
	unsigned int elements;
};
/* Example:
 * string = "--foo\0--bar\0--baz\0"
 * length = 18
 * cuts = {0, 6, 12}
 * elems = 3
 */

int uenv_dup(struct task *t, const char *const __user *_user,
	     struct uenv_array *uenv);
void uenv_free(struct uenv_array *uenv);

#endif
