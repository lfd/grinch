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

#include <grinch/errno.h>
#include <grinch/serial.h>

#define UART_TX			0x0
#define UART_RX			0x0
#define UART_DLL		0x0
#define UART_IER		0x1
#define  UART_IER_RXEN		(1 << 0)
#define UART_DLM		0x1
#define UART_FCR		0x2 /* out: FCR */
#define UART_IIR		0x2 /* in: IIR */
#define UART_LCR		0x3
#define  UART_LCR_8N1		0x03
#define  UART_LCR_DLAB		0x80
#define UART_LSR		0x5
#define  UART_LSR_THRE		0x20

/* DesignWare specific register */
#define DW_UART_USR		0x1f
#define  DW_UART_IIR_BUSY	0x07

static int uart_8250_rcv_handler(struct uart_chip *chip)
{
	unsigned char ch;

	ch = chip->reg_in(chip, UART_RX);
	serial_in(chip, ch);

	return 0;
}

static int uart_8250_dw_rcv_handler(struct uart_chip *chip)
{
	unsigned int err, iir;

	iir = chip->reg_in(chip, UART_IIR);

	err = uart_8250_rcv_handler(chip);
	if (err)
		return err;

	if ((iir & DW_UART_IIR_BUSY) == DW_UART_IIR_BUSY)
		chip->reg_in(chip, DW_UART_USR);

	return 0;
}

static int uart_8250_init(struct uart_chip *chip)
{
	chip->reg_out(chip, UART_LCR, UART_LCR_8N1);
	chip->reg_out(chip, UART_IER, UART_IER_RXEN);
	chip->reg_out(chip, UART_FCR, 0);

	return 0;
}

static bool uart_8250_is_busy(struct uart_chip *chip)
{
	return !(chip->reg_in(chip, UART_LSR) & UART_LSR_THRE);
}

static void uart_8250_write_byte(struct uart_chip *chip, unsigned char c)
{
	chip->reg_out(chip, UART_TX, c);
}

static const struct uart_driver uart_8250 = {
	.init = uart_8250_init,
	.write_byte = uart_8250_write_byte,
	.is_busy = uart_8250_is_busy,
	.rcv_handler = uart_8250_rcv_handler,
};

static const struct uart_driver uart_8250_dw = {
	.init = uart_8250_init,
	.write_byte = uart_8250_write_byte,
	.is_busy = uart_8250_is_busy,
	.rcv_handler = uart_8250_dw_rcv_handler,
};

static const struct of_device_id uart_8250_matches[] = {
	{ .compatible = "ns16550a", .data = &uart_8250, },
	{ .compatible = "uart8250", .data = &uart_8250, },
        { .compatible = "snps,dw-apb-uart", .data = &uart_8250_dw, },
	{},
};

DECLARE_DRIVER(UART8250, "uart-8250", PRIO_1, NULL, uart_probe_generic,
	       uart_8250_matches);
