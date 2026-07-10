/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <grinch/serial.h>
#include <grinch/mmio.h>

#define UARTDR		0x00
#define UARTFR		0x18
#define UARTIBRD	0x24
#define UARTLCR_H	0x2c
#define UARTCR		0x30
#define UARTIMSC	0x38
#define  UARTIMSC_RXIM	(1 << 4)

#define UARTFR_TXFF	(1 << 5)
#define UARTFR_BUSY	(1 << 3)

#define UARTCR_Out2  	(1 << 13)
#define UARTCR_Out1  	(1 << 12)
#define UARTCR_RXE   	(1 << 9)
#define UARTCR_TXE   	(1 << 8)
#define UARTCR_EN	(1 << 0)

#define UARTLCR_H_WLEN	(3 << 5)

static int uart_pl011_init(struct uart_chip *chip)
{
	unsigned long divider = 5;

	mmio_write16(chip->base + UARTCR, 0);
	while (mmio_read8(chip->base + UARTFR) & UARTFR_BUSY)
		cpu_relax();
	mmio_write16(chip->base + UARTIBRD, divider);
	mmio_write8(chip->base + UARTLCR_H, UARTLCR_H_WLEN);
	mmio_write16(chip->base + UARTCR, UARTCR_EN | UARTCR_TXE | UARTCR_RXE |
					  UARTCR_Out1 | UARTCR_Out2);
	mmio_write16(chip->base + UARTIMSC, UARTIMSC_RXIM);

	return 0;
}

static int uart_pl011_rcv_handler(struct uart_chip *chip)
{
	unsigned char ch;

	ch = mmio_read32(chip->base + UARTDR);
	serial_in(chip, ch);

	return 0;
}

static bool uart_pl011_is_busy(struct uart_chip *chip)
{
	/* FIFO full or busy */
	return (mmio_read32(chip->base + UARTFR) &
		(UARTFR_TXFF | UARTFR_BUSY)) != 0;
}

static void uart_pl011_write_byte(struct uart_chip *chip, unsigned char c)
{
	mmio_write32(chip->base + UARTDR, c);
}

static const struct uart_driver uart_pl011 = {
	.init = uart_pl011_init,
	.write_byte = uart_pl011_write_byte,
	.is_busy = uart_pl011_is_busy,
	.rcv_handler = uart_pl011_rcv_handler,
};

static const struct of_device_id uart_pl011_matches[] = {
	{ .compatible = "arm,pl011", .data = &uart_pl011, },
	{ /* sentinel */ }
};

DECLARE_DRIVER(UARTPL011, "uart-pl011", PRIO_1, NULL, uart_probe_generic,
	       uart_pl011_matches);
