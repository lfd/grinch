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

#include "syscalls.h"
#include "vfs.h"

int main(void);

int main(void)
{
	int err;

	printf("Testing Syscalls\n");
	err = test_syscalls();
	if (err)
		goto out;

	printf("Testing VFS API\n");
	err = test_vfs();
	if (err)
		goto out;

out:
	printf("%pe\n", ERR_PTR(err));
	return err;
}
