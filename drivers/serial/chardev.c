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

#include <grinch/errno.h>
#include <grinch/fs.h>
#include <grinch/minmax.h>
#include <grinch/serial.h>
#include <grinch/uaccess.h>
#include <grinch/task.h>

#include <grinch/printk.h>

static ssize_t
serial_read(struct file_handle *h, char *buf, size_t count)
{
	struct uart_chip *c;
	struct device *dev;
	struct file *fp;

	fp = h->fp;
	dev = fp->drvdata;
	c = dev->data;

	return devfs_chardev_read(current_task(), &c->node, h, buf, count);
}

static ssize_t
serial_write(struct file_handle *h, const char *buf, size_t count)
{
	unsigned int this_sz, copied, i;
	struct process *process;
	struct uart_chip *c;
	struct device *dev;
	struct file *fp;
	size_t written;
	char tmp[8];

	fp = h->fp;
	dev = fp->drvdata;
	c = dev->data;

	if (h->flags.is_kernel) {
		for (written = 0; written < count; written++)
			uart_write_byte(c, buf[written]);

		return written;
	}

	process = current_process();
	written = 0;
	while (count) {
		this_sz = min(count, sizeof(tmp));
		copied = copy_from_user(&process->mm, tmp, buf, this_sz);

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

static int
serial_register_reader(struct file_handle *h, char __user *ubuf, size_t count)
{
	struct uart_chip *c;
	struct device *dev;
	struct file *fp;

	fp = h->fp;
	dev = fp->drvdata;
	c = dev->data;

	return devfs_register_reader(h, &c->node, ubuf, count);
}

const struct file_operations serial_fops = {
	.write = serial_write,
	.read = serial_read,
	.register_reader = serial_register_reader,
};
