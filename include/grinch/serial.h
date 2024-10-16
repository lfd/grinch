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

#ifndef _SERIAL_H
#define _SERIAL_H

#include <asm/cpu.h>
#include <asm/spinlock.h>

#include <grinch/fs/devfs.h>
#include <grinch/types.h>

struct uart_chip;

struct uart_driver {
	int (*init)(struct uart_chip *chip);
	void (*write_byte)(struct uart_chip *chip, unsigned char b);
	bool (*is_busy)(struct uart_chip *chip);
	int (*rcv_handler)(struct uart_chip *chip);
};

struct uart_chip {
	const struct uart_driver *driver;
	void *base;
	size_t size;
	u32 irq;

	struct devfs_node node;

	void (*reg_out)(struct uart_chip *chip, unsigned int reg, u32 value);
	u32 (*reg_in)(struct uart_chip *chip, unsigned int reg);

	spinlock_t lock;
};

void uart_write_byte(struct uart_chip *chip, unsigned char b);
void uart_write_char(struct uart_chip *chip, char c);

extern const struct devfs_ops serial_fops;

#include <grinch/driver.h>

int uart_probe_generic(struct device *dev);

void serial_in(struct uart_chip *c, char ch);

#endif /* _SERIAL_H */
