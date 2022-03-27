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
#include <grinch/plic.h>
#include <grinch/printk.h>
#include <grinch/serial.h>

static const struct uart_driver *uart_drivers[] = {
	&uart_sbi,
	&uart_8250,
	&uart_apbuart,
};

struct uart_chip chip = {
	.driver = &uart_sbi,
};

static const char *known_uarts[] = {
	"/soc/uart@10000000",
	"/uart@fc001000",
	"/soc/uart@fc001000",
};

static const struct uart_driver *serial_find_driver(void *fdt, int nodeoff)
{
	const struct uart_driver **d;
	const struct of_compatible *compat_list;
	unsigned int i;
	int err;

	d = uart_drivers;
	for (i = 0; i < ARRAY_SIZE(uart_drivers); i++, d++) {
		compat_list = (*d)->compatible;
		while (compat_list->name) {
			err = fdt_node_check_compatible(fdt, nodeoff,
							compat_list->name);
			if (!err) {
				pr("Found a %s compatible UART\n",
				       compat_list->name);
				return *d;
			}
			compat_list++;
		}
	}

	return ERR_PTR(-ENOENT);
}

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

int serial_init(void)
{
	const struct uart_driver *d;
	struct uart_chip c;
	paddr_t uart_base;
	const int *res;
	u64 uart_size;
	int off, err;
	u32 irq;

	off = fdt_probe_known(_fdt, known_uarts, ARRAY_SIZE(known_uarts));
	if (off <= 0)
		return -ENOENT;

	d = serial_find_driver(_fdt, off);
	if (IS_ERR(d))
		return PTR_ERR(d);


	err = fdt_read_reg(_fdt, off, 0, &uart_base, &uart_size);
	if (err)
		return err;

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
		plic_register_handler(irq, (void*)chip.driver->rcv_handler,
				      &chip);
		plic_enable_irq(this_cpu_id(), irq, 5, 4);
	}

	return err;
}
