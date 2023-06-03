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
#include <grinch/serial.h>

static int uart_dummy_rcv_handler(struct uart_chip *chip)
{
	return 0;
}

static int uart_dummy_init(struct uart_chip *chip)
{
	return 0;
}

static bool uart_dummy_is_busy(struct uart_chip *chip)
{
	return false;
}

static void uart_dummy_write_byte(struct uart_chip *chip, unsigned char c)
{
}

const struct uart_driver uart_dummy = {
	.init = uart_dummy_init,
	.write_byte = uart_dummy_write_byte,
	.is_busy = uart_dummy_is_busy,
	.rcv_handler = uart_dummy_rcv_handler,
};
