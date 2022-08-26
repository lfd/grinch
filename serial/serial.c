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

#define dbg_fmt(x) "serial: " x

#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/percpu.h>
#include <grinch/irqchip.h>
#include <grinch/printk.h>
#include <grinch/serial.h>

struct uart_chip chip = {
	.driver = &uart_sbi,
};

static void uart_write_char(char ch)
{
	while (chip.driver->is_busy(&chip))
		cpu_relax();
	chip.driver->write_char(&chip, ch);
}

void _puts(const char *msg)
{
	char c;

	while (1) {
		c = *msg++;
		if (!c)
			break;

		if (c == '\n')
			uart_write_char('\r');
		
		uart_write_char(c);
	}
}

void serial_in(char ch)
{
	pr("STDIN rcvd: %c\n", ch);
}

static const struct of_device_id of_match[] = {
	{ .compatible = "ns16550a", .data = &uart_8250, },
	{ .compatible = "uart8250", .data = &uart_8250, },
	{ .compatible = "gaisler,apbuart", .data = &uart_apbuart, },
	{ /* sentinel */}
};

int serial_init(void)
{
	const struct of_device_id *match;
	const struct uart_driver *d;
	struct uart_chip c;
	paddr_t uart_base;
	const int *res;
	u64 uart_size;
	int off, err;
	u32 irq;

	off = fdt_find_device(_fdt, "/soc", of_match, &match);
	if (off <= 0) {
		pr("WARNING: No UART found. Remaining on SBI console\n");
		return -ENOENT;
	}

	d = match->data;

	err = fdt_read_reg(_fdt, off, 0, &uart_base, &uart_size);
	if (err)
		return err;
	pr("Found %s UART@0x%llx\n", match->compatible, uart_base);

	res = fdt_getprop(_fdt, off, "interrupts", &err);
	if (IS_ERR(res)) {
		irq = 0;
		ps("No IRQ found!\n");
	} else {
		irq = fdt32_to_cpu(*res);
		pr("UART: using IRQ %d\n", irq);
	}

	c.driver = d;
	c.irq = irq;

	c.base = ioremap(uart_base, uart_size);
	if (IS_ERR(c.base))
		return PTR_ERR(c.base);

	err = d->init(&c);
	if (err)
		return err;

	/* activate as default chip */
	chip = c;

	if (irq) {
		irq_register_handler(irq, (void*)chip.driver->rcv_handler,
				     &chip);
		irqchip_enable_irq(this_cpu_id(), irq, 5, 4);
	}

	return err;
}
