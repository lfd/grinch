/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/errno.h>
#include <grinch/mmio.h>
#include <grinch/serial.h>

#define UARTLITE_RX		0x0
#define UARTLITE_TX		0x4
#define UARTLITE_STATUS		0x8
#define UARTLITE_CTRL		0xc

#define UARTLITE_STATUS_TX_EMPTY	(1 << 2)
#define UARTLITE_STATUS_TX_FULL		(1 << 3)

static int uart_uartlite_rcv_handler(struct uart_chip *chip)
{
	unsigned char ch;

	ch = mmio_read8(chip->base + UARTLITE_RX);
	serial_in(ch);

	return 0;
}

static int uart_uartlite_init(struct uart_chip *chip)
{
	return 0;
}

static bool uart_uartlite_is_busy(struct uart_chip *chip)
{
	return !!(mmio_read32(chip->base + UARTLITE_STATUS)
		  & UARTLITE_STATUS_TX_FULL);
}

static void uart_uartlite_write_byte(struct uart_chip *chip, unsigned char c)
{
	mmio_write32(chip->base + UARTLITE_TX, c);
}

const struct uart_driver uart_uartlite = {
	.init = uart_uartlite_init,
	.write_byte = uart_uartlite_write_byte,
	.is_busy = uart_uartlite_is_busy,
	.rcv_handler = uart_uartlite_rcv_handler,
};
