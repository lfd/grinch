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

#include <grinch/errno.h>
#include <grinch/mmio.h>
#include <grinch/paging.h>
#include <grinch/serial.h>

#define UART_TX			0x0
#define UART_RX			0x0
#define UART_DLL		0x0
#define UART_IER		0x1
#define  UART_IER_RXEN		(1 << 0)
#define UART_DLM		0x1
#define UART_LCR		0x3
#define  UART_LCR_8N1		0x03
#define  UART_LCR_DLAB		0x80
#define UART_LSR		0x5
#define  UART_LSR_THRE		0x20

static int uart_8250_rcv_handler(struct uart_chip *chip)
{
	unsigned char ch;

	ch = mmio_read8(chip->base + UART_RX);
	serial_in(ch);

	return 0;
}

static int uart_8250_init(struct uart_chip *chip)
{
	// FIXME: Only works with register width 8
	mmio_write8(chip->base + UART_LCR, UART_LCR_8N1);
	mmio_write8(chip->base + UART_IER, UART_IER_RXEN);

	return 0;
}

static bool uart_8250_is_busy(struct uart_chip *chip)
{
	return !(mmio_read8(chip->base + UART_LSR) & UART_LSR_THRE);
}

static void uart_8250_write_char(struct uart_chip *chip, char c)
{
	mmio_write8(chip->base + UART_TX, c);
}

const struct uart_driver uart_8250 = {
	.init = uart_8250_init,
	.write_char = uart_8250_write_char,
	.is_busy = uart_8250_is_busy,
	.rcv_handler = uart_8250_rcv_handler,
};
