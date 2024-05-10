/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/fs/devfs.h>
#include <grinch/init.h>
#include <grinch/minmax.h>
#include <grinch/uaccess.h>
#include <grinch/task.h>

#include <grinch/arch/sbi.h>

static ssize_t
sbi_write(struct devfs_node *node, void *user, struct file_handle *fh,
	  const char *buf, size_t count)
{
	unsigned int this_sz, copied, i;
	struct task *task;
	size_t written;
	char tmp[8];

	if (fh->flags.is_kernel) {
		for (written = 0; written < count; written++)
			sbi_console_putchar(buf[written]);
		return written;
	}

	task = current_task();
	written = 0;
	while (count) {
		this_sz = min(count, sizeof(tmp));
		copied = copy_from_user(task, tmp, buf, this_sz);

		count -= this_sz;
		buf += this_sz;
		written += this_sz;

		for (i = 0; i < copied; i++)
			sbi_console_putchar(tmp[i]);

		if (copied != this_sz)
			return written;
	}

	return written;
}

static ssize_t
sbi_read(struct devfs_node *node, void *user, struct file_handle *fh,
	 char *buf, size_t count)
{
	struct sbiret ret;
	char c;

	if (!count)
		return 0;

	ret = sbi_console_getchar();
	if (ret.error < 0)
		panic("sbi_console_getchar: %ld\n", ret.error);

	c = (char)ret.error;

	if (fh->flags.is_kernel)
		*buf = c;
	else
		copy_to_user(current_task(), buf, &c, 1);

	return 1;
}

static const struct devfs_ops sbi_fops = {
	.write = sbi_write,
	.read = sbi_read,
};

static struct devfs_node sbi_node = {
	.name = "ttySBI",
	.type = DEVFS_REGULAR,
	.fops = &sbi_fops,
};

static int __init tty_sbi_init(void)
{
	int err;

	err = devfs_node_init(&sbi_node);
	if (err)
		return err;

	err = devfs_node_register(&sbi_node);
	if (err)
		devfs_node_deinit(&sbi_node);

	return err;
}

DECLARE_DRIVER(TTYSBI, PRIO_1, tty_sbi_init, NULL, NULL)
