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

#define dbg_fmt(x)	"vfs: " x

#include <string.h>

#include <grinch/errno.h>
#include <grinch/initrd.h>
#include <grinch/printk.h>
#include <grinch/vfs.h>

void *vfs_read_file(const char *pathname, size_t *len)
{
	const char *ird = "initrd:";

	/* We only support the initrd file system at the moment */
	if (strncmp(pathname, ird, strlen(ird)))
		return ERR_PTR(-ENOSYS);

	return initrd_read_file(pathname + strlen(ird), len);
}

int __init vfs_init(void)
{
	int err;

	err = initrd_init();
	if (err == -ENOENT)
		pri("No ramdisk found\n");
	else if (err)
		return err;

	return 0;
}
