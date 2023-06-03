/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/cpu.h>
#include <grinch/errno.h>
#include <grinch/mmio.h>
#include <grinch/serial.h>

#define UART_STATUS_DR   0x00000001	/* Data Ready */
#define UART_STATUS_TSE  0x00000002	/* TX Send Register Empty */
#define UART_STATUS_THE  0x00000004	/* TX Hold Register Empty */
#define UART_STATUS_BR   0x00000008	/* Break Error */
#define UART_STATUS_OE   0x00000010	/* RX Overrun Error */
#define UART_STATUS_PE   0x00000020	/* RX Parity Error */
#define UART_STATUS_FE   0x00000040	/* RX Framing Error */
#define UART_STATUS_ERR  0x00000078	/* Error Mask */

#define UART_CTRL_RE     0x00000001	/* Receiver enable */
#define UART_CTRL_TE     0x00000002	/* Transmitter enable */
#define UART_CTRL_RI     0x00000004	/* Receiver interrupt enable */
#define UART_CTRL_TI     0x00000008	/* Transmitter irq */
#define UART_CTRL_PS     0x00000010	/* Parity select */
#define UART_CTRL_PE     0x00000020	/* Parity enable */
#define UART_CTRL_FL     0x00000040	/* Flow control enable */
#define UART_CTRL_LB     0x00000080	/* Loopback enable */

struct apbuart {
	u32 data;
	u32 status;
	u32 ctrl;
	u32 scaler;
} __attribute__((packed));

static void apbuart_write_byte(struct uart_chip *chip, unsigned char ch)
{
	struct apbuart *uart = chip->base;

	mmio_write32(&uart->data, ch);
}

static bool apbuart_is_busy(struct uart_chip *chip)
{
	struct apbuart *uart = chip->base;
	u32 status;

	status = mmio_read32(&uart->status);

	return !(status & UART_STATUS_THE);
}

static int apbuart_rcv(struct uart_chip *chip)
{
	struct apbuart *uart = chip->base;
	u32 status;
	char ch;

	status = mmio_read32(&uart->status);
	while (status & UART_STATUS_DR) {
		mmio_write32(&uart->status, 0);

		ch = mmio_read32(&uart->data);
		serial_in(ch);

		status = mmio_read32(&uart->status);
	}

	return 0;
}

static int apbuart_init(struct uart_chip *chip)
{
	struct apbuart *uart = chip->base;
	unsigned int cr;

	mmio_write32(&uart->status, 0);

	cr = mmio_read32(&uart->ctrl);
	cr |= UART_CTRL_RE | UART_CTRL_TE | UART_CTRL_RI;
	cr &= ~UART_CTRL_TI;
	mmio_write32(&uart->ctrl, cr);

	return 0;
}

const struct uart_driver uart_apbuart = {
	.init = apbuart_init,
	.write_byte = apbuart_write_byte,
	.is_busy = apbuart_is_busy,
	.rcv_handler = apbuart_rcv,
};
