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

#define dbg_fmt(x)	"fs-syscall: " x

#include <grinch/errno.h>
#include <grinch/syscall.h>
#include <grinch/types.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/vma.h>
#include <grinch/uaccess.h>

#define MAX_PATHLEN	64

unsigned long sys_open(const char __user *path, int oflag)
{
	return -EPERM;
}

unsigned long sys_close(int fd)
{
	return -EPERM;
}

unsigned long sys_read(int fd, char __user *buf, size_t count)
{
	return -EPERM;
}

unsigned long sys_write(int fd, const char __user *buf, size_t count)
{
#define BLEN	63

	char tmp[BLEN + 1];
	unsigned long sz;
	int err;

	if (fd != 1)
		return -ENOENT;

	while (count) {
		sz = count < BLEN ? count : BLEN;
		err = copy_from_user(&current_process()->mm, tmp, buf, sz);
		if (err < 0)
			return err;
		tmp[sz] = 0;
		_puts(tmp);

		count -= err;
		buf += err;
	}

	return 0;
}

int sys_execve(const char __user *pathname, char *const __user argv[],
	       char *const __user envp[])
{
	struct task *this;
	char buf[MAX_PATHLEN];
	long ret;

	this = current_task();
	ret = ustrncpy(buf, pathname, sizeof(buf));
	/* pathname too long */
	if (unlikely(ret == sizeof(buf)))
		return -ERANGE;
	else if (unlikely(ret < 0))
		return ret;

	uvmas_destroy(this->process);
	this_per_cpu()->pt_needs_update = true;

	return process_from_fs(this, buf);
}
