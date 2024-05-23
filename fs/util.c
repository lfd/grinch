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

#include <grinch/fs/util.h>

#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/string.h>
#include <grinch/task.h>
#include <grinch/fs/util.h>
#include <grinch/uaccess.h>

static int pathname_sanitise_dir(char *pathname, bool *_must_dir)
{
	char *dst, *pos;
	bool must_dir, is_dot;

	/* Don't support empty paths */
	if (pathname[0] == '\0')
		return -EINVAL;

	/* Special treatment for "/" */
	if (pathname[1] == '\0') {
		must_dir = true;
		goto out;
	}

	must_dir = false;
	is_dot = false;
	pos = dst = pathname;
	for (;;) {
		if (pos[0] == '\0') {
			if (is_dot)
				return -EINVAL;

			if (dst[-1] == '/') {
				dst[-1] = 0;
				must_dir = true;
			} else
				*dst = 0;
			break;
		}

		if (pos[0] == '.') {
			if (pos[-1] == '/')
				is_dot = true;
		} else if (pos[0] != '/')
			is_dot = false;

		if (pos[0] == '/') {
			if (is_dot)
				return -EINVAL;
			else if (dst[-1] == '/')
				goto skip;
		}
		*dst = pos[0];
		dst++;

skip:
		pos++;
		while (pos[0] == '/' && pos[1] == '/')
			pos++;
	}

out:
	if (_must_dir)
		*_must_dir = must_dir;

	if (!*pathname) {
		pathname[0] = '/';
		pathname[1] = 0;
	}

	return 0;
}

char *pathname_from_user(const char __user *_pathname, bool *must_dir)
{
	char pathname[MAX_PATHLEN];
	ssize_t len;
	int err;

	len = ustrncpy(pathname, _pathname, MAX_PATHLEN);
	if (unlikely(len < 0)) {
		err = (len == -ERANGE) ? -ENAMETOOLONG : len;
		return ERR_PTR(err);
	}

	err = pathname_sanitise_dir(pathname, must_dir);
	if (err)
		return ERR_PTR(err);

	return kstrdup(pathname);
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
