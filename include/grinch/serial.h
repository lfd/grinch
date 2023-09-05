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
	u32 irq;
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

extern struct uart_chip uart_default;

extern const struct uart_driver uart_dummy;
extern const struct uart_driver uart_apbuart;
extern const struct uart_driver uart_8250;
#ifdef ARCH_RISCV
extern const struct uart_driver uart_sbi;
extern const struct uart_driver uart_uartlite;
#elif defined(ARCH_ARM64)
extern const struct uart_driver uart_bcm2835_aux;
#endif

void serial_in(char ch);
int serial_init(const struct uart_driver *d, paddr_t uart_base, u64 uart_size, u32 irq);
int serial_init_fdt(void);

#endif /* _SERIAL_H */
