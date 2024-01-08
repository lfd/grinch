/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "serial: " x

#include <asm/cpu.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/percpu.h>
#include <grinch/irqchip.h>
#include <grinch/mmio.h>
#include <grinch/printk.h>
#include <grinch/serial.h>

struct uart_chip uart_default = {
#if defined(ARCH_RISCV)
	.driver = &uart_sbi,
#elif defined(ARCH_ARM64)
	.driver = &uart_dummy,
#endif
};

void serial_in(char ch)
{
	pr("STDIN rcvd: %c\n", ch);
}

static const struct of_device_id of_match[] = {
	{ .compatible = "ns16550a", .data = &uart_8250, },
	{ .compatible = "uart8250", .data = &uart_8250, },
	{ .compatible = "gaisler,apbuart", .data = &uart_apbuart, },
#if defined(ARCH_RISCV)
	{ .compatible = "riscv,axi-uart-1.0", .data = &uart_uartlite, },
	{ .compatible = "xlnx,opb-uartlite-1.00.b", .data = &uart_uartlite, },
	{ .compatible = "xlnx,opb-uartlite-1.00.a", .data = &uart_uartlite, },
#endif
	{ /* sentinel */}
};

static void reg_out_mmio8(struct uart_chip *chip, unsigned int reg, u32 value)
{
	mmio_write8(chip->base + reg, value);
}

static u32 reg_in_mmio8(struct uart_chip *chip, unsigned int reg)
{
	return mmio_read8(chip->base + reg);
}

static void reg_out_mmio32(struct uart_chip *chip, unsigned int reg, u32 value)
{
	mmio_write32(chip->base + reg * 4, value);
}

static u32 reg_in_mmio32(struct uart_chip *chip, unsigned int reg)
{
	return mmio_read32(chip->base + reg * 4);
}

int __init serial_init(const struct uart_driver *d, paddr_t uart_base,
		       u64 uart_size, int io_width, u32 irq)
{
	int err;
	struct uart_chip c;
	bool flush;

	c.driver = d;
	c.irq = irq;

	switch (io_width) {
		case 1:
			c.reg_in = reg_in_mmio8;
			c.reg_out = reg_out_mmio8;
			break;
		case 4:
			c.reg_in = reg_in_mmio32;
			c.reg_out = reg_out_mmio32;
			break;

		default:
			pr("Invalid IO width: %d\n", io_width);
			return -EINVAL;
	}

	c.base = ioremap(uart_base, uart_size);
	if (IS_ERR(c.base))
		return PTR_ERR(c.base);

	err = d->init(&c);
	if (err)
		return err;

	/* activate as default chip */
	flush = uart_default.driver == &uart_dummy;
	uart_default = c;
	if (flush)
		console_flush();

	if (irq != IRQ_INVALID) {
		pr("UART: using IRQ %d\n", irq);
		err = irq_register_handler(irq, (void*)uart_default.driver->rcv_handler,
					   &uart_default);
		if (err) {
			pr("Unable to register IRQ %d (%d)\n", irq, err);
			return err;
		}
		err = irqchip_enable_irq(this_cpu_id(), irq, 5, 4);
		if (err) {
			pr("Unable to enable IRQ %d\n", irq);
			return err;
		}
	} else {
		ps("No IRQ found!\n");
	}

	return err;
}

int __init serial_init_fdt(void)
{
	const struct of_device_id *match;
	const struct uart_driver *d;
	int off, err, io_width = 1;
	paddr_t uart_base;
	const int *res;
	u64 uart_size;
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

	res = fdt_getprop(_fdt, off, "reg-io-width", &err);
	if (err > 0)
		io_width = fdt32_to_cpu(*res);
	pr("Found %s UART@0x%llx\n", match->compatible, uart_base);

	res = fdt_getprop(_fdt, off, "interrupts", &err);
	if (IS_ERR(res))
		irq = 0;
	else
		irq = fdt32_to_cpu(*res);

	return serial_init(d, uart_base, uart_size, io_width, irq);
}
