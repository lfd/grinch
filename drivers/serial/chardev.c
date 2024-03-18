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
	struct process *process;
	struct uart_chip *chip;
	unsigned long copied;
	struct device *dev;
	struct ringbuf *rb;
	unsigned int cnt;
	struct file *fp;
	ssize_t ret;
	char *src;

	if (!count)
		return 0;

	if (h->flags.is_kernel)
		BUG();

	process = current_process();

	fp = h->fp;
	dev = fp->drvdata;
	chip = dev->data;

	ret = 0;
	rb = &chip->rb;

	spin_lock(&chip->lock);
	do {
		src = ringbuf_read(rb, &cnt);
		if (!cnt)
			break;
		cnt = min(cnt, count);
		ringbuf_consume(rb, cnt);

		copied = copy_to_user(&process->mm, buf, src, cnt);

		buf += cnt;
		count -= cnt;
		ret += copied;
		if (copied != cnt)
			break;
	} while (count);
	spin_unlock(&chip->lock);

	return ret;
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

const struct file_operations serial_fops = {
	.write = serial_write,
	.read = serial_read,
};
