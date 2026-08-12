/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/errno.h>
#include <grinch/serial.h>

/*
 * Register indices for the LiteX 32-bit CSR bus (reg-io-width = <4>):
 * the mmio32 accessor maps index N to byte offset N*4.
 */
#define LITEUART_RXTX       0
#define LITEUART_TXFULL     1
#define LITEUART_RXEMPTY    2
#define LITEUART_EV_STATUS  3
#define LITEUART_EV_PENDING 4
#define LITEUART_EV_ENABLE  5

#define EV_TX	(1 << 0)
#define EV_RX	(1 << 1)

static int uart_liteuart_rcv_handler(struct uart_chip *chip)
{
	unsigned char ch;

	while (!chip->reg_in(chip, LITEUART_RXEMPTY)) {
		ch = chip->reg_in(chip, LITEUART_RXTX);
		chip->reg_out(chip, LITEUART_EV_PENDING, EV_RX);
		serial_in(chip, ch);
	}

	return 0;
}

static int uart_liteuart_init(struct uart_chip *chip)
{
	chip->reg_out(chip, LITEUART_EV_PENDING, EV_TX | EV_RX);
	chip->reg_out(chip, LITEUART_EV_ENABLE, EV_RX);
	return 0;
}

static bool uart_liteuart_is_busy(struct uart_chip *chip)
{
	return !!chip->reg_in(chip, LITEUART_TXFULL);
}

static void uart_liteuart_write_byte(struct uart_chip *chip, unsigned char c)
{
	chip->reg_out(chip, LITEUART_RXTX, c);
}

static const struct uart_driver uart_liteuart = {
	.init		= uart_liteuart_init,
	.write_byte	= uart_liteuart_write_byte,
	.is_busy	= uart_liteuart_is_busy,
	.rcv_handler	= uart_liteuart_rcv_handler,
};

static const struct of_device_id uart_liteuart_matches[] = {
	{ .compatible = "litex,liteuart", .data = &uart_liteuart, },
	{ /* Sentinel */ }
};

DECLARE_DRIVER(liteuart, "liteuart", PRIO_1, NULL, uart_probe_generic,
	       uart_liteuart_matches);
