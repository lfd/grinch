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

#define dbg_fmt(x)	"console: " x

#include <grinch/arch/console.h>

#include <grinch/bootparam.h>
#include <grinch/console.h>
#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/init.h>
#include <grinch/printk.h>
#include <grinch/ringbuf.h>
#include <grinch/serial.h>
#include <grinch/vsprintf.h>

#ifdef ARCH_RISCV
#define DEFAULT_CONSOLE	"ttySBI"
#endif

static char __console_buffer[2048];
static struct ringbuf console_ringbuf = {
	.buf = __console_buffer,
	.size = sizeof(__console_buffer),
};

static char console_device[DEVFS_MAX_LEN_NAME];

static struct file_handle kstdout = {
	.flags = {
		.is_kernel = true,
		.may_write = true,
		.may_read = true,
	},
};

static void console_parse(const char *str)
{
	strncpy(console_device, str, sizeof(console_device - 1));
}
bootparam(console, console_parse);

static inline void kstdout_write(const char *str, unsigned int len)
{
	kstdout.fp->fops->write(&kstdout, str, len);
}

static inline void rb_write(char c)
{
	ringbuf_write(&console_ringbuf, c);
}

#if 0
static void ringbuf_flush(void)
{
	unsigned int sz;
	char *src;

	do {
		src = ringbuf_read(&console_ringbuf, &sz);
		kstdout_write(src, sz);
		ringbuf_consume(&console_ringbuf, sz);
	} while (sz);
}
#endif

void console_puts(const char *str)
{
	unsigned int i;

	if (!kstdout.fp)
		for (i = 0; str[i]; i++) {
			rb_write(str[i]);
			arch_early_dbg(str[i]);
		}
	else {
		for (i = 0; str[i]; i++)
			rb_write(str[i]);
		kstdout_write(str, i);
	}
}

int __init console_init(void)
{
	const char *stdoutpath, *src;
	struct uart_chip *chip;
	struct device *dev;
	struct file *fp;
	int err, node;

	if (*console_device) {
		src = console_device;
		goto open_console;
	}

	node = fdt_path_offset(_fdt, "/chosen");
	if (node < 0) {
		pri("No chosen node in device-tree.\n");
		goto remain;
	}

	stdoutpath = fdt_getprop(_fdt, node, ISTR("stdout-path"), &err);
	if (!stdoutpath) {
		pri("No stdout-path in chosen node\n");
		goto remain;
	}
	pri("stdout-path: %s\n", stdoutpath);

	// FIXME: _this_ is hacky.
	dev = device_find_of_path(stdoutpath);
	if (!dev)
		goto remain;

	chip = dev->data;
	src = chip->node.name;

	goto open_console;

remain:
	pri("Remaining on default console\n");
#ifdef ARCH_RISCV
	src = DEFAULT_CONSOLE;
	goto open_console;
#else
	return -ENOENT;
#endif

open_console:
	pri("Switching kernel console to %s\n", src);
	err = devfs_create_symlink(ISTR("console"), src);
	if (err)
		return err;

	fp = file_open_at(NULL, ISTR(DEVICE_NAME("console")));
	if (IS_ERR(fp)) {
		pri("Error opening console %s: %pe\n", src, fp);
		return PTR_ERR(fp);
	}

	kstdout.fp = fp;
	return 0;
}
