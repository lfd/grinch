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

#include <string.h>

#include <grinch/fs/util.h>

#include <grinch/errno.h>
#include <grinch/task.h>
#include <grinch/fs/util.h>
#include <grinch/uaccess.h>

bool pathname_sanitise_dir(char *pathname)
{
	size_t len;

	len = strlen(pathname);
	if (len && pathname[len - 1] == '/') {
		pathname[len - 1] = 0;
		return true;
	}

	return false;
}

int copy_dirent(struct grinch_dirent __mayuser *udent, bool is_kernel,
		struct grinch_dirent *src, const char *name,
		unsigned int size)
{
	unsigned long copied;
	struct task *task;
	size_t nlen;

	nlen = strlen(name) + 1;
	if (size < nlen + sizeof(*src))
		return -EINVAL;

	if (is_kernel) {
		memcpy(udent, src, sizeof(*src));
		memcpy(udent->name, name, nlen);
	} else {
		task = current_task();
		copied = copy_to_user(task, udent, src, sizeof(*src));
		if (copied != sizeof(*src))
			return -EFAULT;
		copied = copy_to_user(task, udent->name, name, nlen);
		if (copied != nlen)
			return -EFAULT;
	}

	return 0;
}
