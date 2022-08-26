/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _SERIAL_H
#define _SERIAL_H

#include <grinch/types.h>

struct of_compatible {
	const char *name;
};

struct uart_chip;

struct uart_driver {
	int (*init)(struct uart_chip *chip);
	void (*write_char)(struct uart_chip *chip, char ch);
	bool (*is_busy)(struct uart_chip *chip);
	int (*rcv_handler)(struct uart_chip *chip);

	struct of_compatible compatible[];
};

struct uart_chip {
	const struct uart_driver *driver;
	void *base;
	u32 irq;
};

extern const struct uart_driver uart_apbuart;
extern const struct uart_driver uart_8250;
extern const struct uart_driver uart_sbi;

void serial_in(char ch);
int serial_init(void);

#endif /* _SERIAL_H */
