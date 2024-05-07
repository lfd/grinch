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

#ifndef _FS_INITRD_H
#define _FS_INITRD_H

/* used for passing ram disk to VMs */
struct initrd {
	paddr_t pstart;
	size_t size;
	const void *vbase;
};

extern struct initrd initrd;

/* Initrdfs variables and helpers */
extern const struct file_system initrdfs;

int initrd_init(void);

#endif /* _FS_INITRD_H */
