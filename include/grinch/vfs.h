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

#ifndef _VFS_H
#define _VFS_H

struct initrd {
	paddr_t pstart;
	size_t size;
	const void *vbase;
};

extern struct initrd initrd;

void *vfs_read_file(const char *pathname, size_t *len);

int vfs_init(void);

#endif /* _VFS_H */
