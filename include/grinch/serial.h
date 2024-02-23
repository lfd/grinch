/*
 * Grinch, a minimalist operating system
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
#include <asm/cpu.h>

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

	void (*reg_out)(struct uart_chip *chip, unsigned int reg, u32 value);
	u32 (*reg_in)(struct uart_chip *chip, unsigned int reg);
};

static inline void uart_write_byte(struct uart_chip *chip, unsigned char b)
{
	while (chip->driver->is_busy(chip))
		cpu_relax();
	chip->driver->write_byte(chip, b);
}

static inline void uart_write_char(struct uart_chip *chip, char c)
{
	if (c == '\n')
		uart_write_byte(chip, '\r');
	
	uart_write_byte(chip, c);
}

extern struct uart_chip *uart_stdout;

extern const struct uart_driver uart_dummy;
#ifdef ARCH_RISCV
extern const struct uart_driver uart_sbi;
#endif

#include <grinch/driver.h>
int uart_init(struct device *dev);
void uart_deinit(struct device *dev);

int uart_probe_generic(struct device *dev);

void serial_in(char ch);
int serial_init(const struct uart_driver *d, paddr_t uart_base, u64 uart_size,
		int io_width, u32 irq);
int serial_init_fdt(void);

#endif /* _SERIAL_H */
