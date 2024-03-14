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

#ifndef _INITRD_H
#define _INITRD_H

struct initrd {
	paddr_t pstart;
	size_t size;
	const void *vbase;
};

extern struct initrd initrd;

int initrd_init(void);

/* /initrd mountpoint */
extern const struct file_system initrdfs;

#endif /* _INITRD_H */
