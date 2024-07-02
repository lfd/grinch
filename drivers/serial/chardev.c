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

#define dbg_fmt(x) "serial: " x

#include <grinch/device.h>
#include <grinch/errno.h>
#include <grinch/minmax.h>
#include <grinch/serial.h>
#include <grinch/uaccess.h>
#include <grinch/task.h>

#include <grinch/printk.h>

static ssize_t
serial_write(struct devfs_node *node, struct file_handle *fh, const char *buf,
	     size_t count)
{
	unsigned int this_sz, copied, i;
	struct uart_chip *c;
	struct device *dev;
	struct task *task;
	size_t written;
	char tmp[8];

	dev = node->drvdata;
	c = dev->data;

	if (fh->flags.is_kernel) {
		for (written = 0; written < count; written++)
			uart_write_byte(c, buf[written]);

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
			uart_write_byte(c, tmp[i]);

		if (copied != this_sz)
			return written;
	}

	return written;
}

const struct devfs_ops serial_fops = {
	.write = serial_write,
};
