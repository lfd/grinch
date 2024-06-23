/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/errno.h>
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

	ch = chip->reg_in(chip, UARTLITE_RX);
	serial_in(chip, ch);

	return 0;
}

static int uart_uartlite_init(struct uart_chip *chip)
{
	return 0;
}

static bool uart_uartlite_is_busy(struct uart_chip *chip)
{
	return !!(chip->reg_in(chip, UARTLITE_STATUS)
		  & UARTLITE_STATUS_TX_FULL);
}

static void uart_uartlite_write_byte(struct uart_chip *chip, unsigned char c)
{
	chip->reg_out(chip, UARTLITE_TX, c);
}

static const struct uart_driver uart_uartlite = {
	.init = uart_uartlite_init,
	.write_byte = uart_uartlite_write_byte,
	.is_busy = uart_uartlite_is_busy,
	.rcv_handler = uart_uartlite_rcv_handler,
};

static const struct of_device_id uart_uartlite_matches[] = {
	{ .compatible = "riscv,axi-uart-1.0", .data = &uart_uartlite, },
	{ .compatible = "xlnx,opb-uartlite-1.00.b", .data = &uart_uartlite, },
	{ .compatible = "xlnx,opb-uartlite-1.00.a", .data = &uart_uartlite, },
	{ /* Sentinel */ }
};

DECLARE_DRIVER(uartlite, "uartlite", PRIO_1, NULL, uart_probe_generic,
	       uart_uartlite_matches);
