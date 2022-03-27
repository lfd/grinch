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
#include <grinch/serial.h>
#include <grinch/sbi.h>

static bool uart_sbi_busy(struct uart_chip *chip)
{
	return false;
}

static void uart_sbi_write_char(struct uart_chip *chip, char ch)
{
	sbi_console_putchar(ch);
}

static int uart_sbi_init(struct uart_chip *chip)
{
	return 0;
}

const struct uart_driver uart_sbi = {
	.init = uart_sbi_init,
	.write_char = uart_sbi_write_char,
	.is_busy = uart_sbi_busy,
	.compatible = {
		{
			.name ="sbi",
		},
		{ },
	},
};
