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

#include "vfs.h"

int test_vfs(void)
{
	int err;

	printf(" -> devfs\n");
	err = test_devfs();
	if (err)
		return err;

	printf(" -> initrd\n");
	err = test_initrd();
	if (err)
		return err;

	printf("Testing tmpfs\n");
	err = test_tmpfs();
	if (err)
		return err;

	return err;
}
