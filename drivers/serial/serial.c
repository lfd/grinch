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

#define dbg_fmt(x) "serial: " x

#include <asm/cpu.h>
#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/percpu.h>
#include <grinch/irqchip.h>
#include <grinch/mmio.h>
#include <grinch/printk.h>
#include <grinch/serial.h>

static struct uart_chip uart_default = {
#if defined(ARCH_RISCV)
	.driver = &uart_sbi,
#elif defined(ARCH_ARM64)
	.driver = &uart_dummy,
#endif
};

struct uart_chip *uart_stdout = &uart_default;

void serial_in(char ch)
{
	pr("STDIN rcvd: %c\n", ch);
}

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

static int serial_rcv(void *_c)
{
	struct uart_chip *c = _c;
	int err;

	err = c->driver->rcv_handler(c);

	return err;
}

int __init uart_probe_generic(struct device *dev)
{
	const struct uart_driver *d;
	struct uart_chip *c;
	int err;

	err = uart_init(dev);
	if (err)
		return err;

	d = dev->of.match->data;

	c = dev->data;
	c->driver = d;
	err = d->init(c);
	if (err) {
		uart_deinit(dev);
		return err;
	}

	return err;
}

void __init uart_deinit(struct device *dev)
{
	struct uart_chip *c;
	int err;

	c = dev->data;
	dev->data = NULL;
	if (!c)
		return;

	if (c->base) {
		err = iounmap(c->base, c->size);
		if (err)
			pr("Error during unmap\n");
	}

	kfree(c);
}

int __init uart_init(struct device *dev)
{
	int err;

	paddr_t uart_base;
	u64 uart_size;
	u32 irq;
	int io_width = 1;
	const int *res;
	struct uart_chip *c;

	err = fdt_read_reg(_fdt, dev->of.node, 0, &uart_base, &uart_size);
	if (err)
		return err;

	res = fdt_getprop(_fdt, dev->of.node, ISTR("reg-io-width"), &err);
	if (err > 0)
		io_width = fdt32_to_cpu(*res);

	res = fdt_getprop(_fdt, dev->of.node, ISTR("interrupts"), &err);
	if (IS_ERR(res))
		irq = 0;
	else
		irq = fdt32_to_cpu(*res);

	c = kzalloc(sizeof(*c));
	if (!c)
		return -ENOMEM;
	dev->data = c;

	spin_init(&c->lock);

	switch (io_width) {
		case 1:
			c->reg_in = reg_in_mmio8;
			c->reg_out = reg_out_mmio8;
			break;
		case 4:
			c->reg_in = reg_in_mmio32;
			c->reg_out = reg_out_mmio32;
			break;

		default:
			pri("Invalid IO width: %d\n", io_width);
			err = -EINVAL;
			goto error_out;
	}

	c->base = ioremap(uart_base, uart_size);
	c->size = uart_size;
	if (IS_ERR(c->base)) {
		err = PTR_ERR(c->base);
		goto error_out;
	}

	c->irq = irq;
	if (irq != IRQ_INVALID) {
		pri("UART: using IRQ %d\n", irq);
		err = irq_register_handler(irq, serial_rcv, c);
		if (err) {
			pri("Unable to register IRQ %d (%pe)\n",
			   irq, ERR_PTR(err));
			goto error_out;
		}
		err = irqchip_enable_irq(this_cpu_id(), irq, 5, 4);
		if (err) {
			pri("Unable to enable IRQ %d (%pe)\n",
			   irq, ERR_PTR(err));
			goto error_out;
		}
	} else
		pri("No IRQ found!\n");

	return 0;

error_out:
	uart_deinit(dev);
	return err;
}

int __init serial_init_fdt(void)
{
	int err, node;
	struct device *dev;
	const char *stdoutpath;

	node = fdt_path_offset(_fdt, "/chosen");
	if (node < 0) {
		pri("No chosen node in device-tree.\n");
		goto remain;
	}

	stdoutpath = fdt_getprop(_fdt, node, ISTR("stdout-path"), &err);
	if (!stdoutpath) {
		pri("No stdout-path in chosen node\n");
		goto remain;
	}
	pri("stdout-path: %s\n", stdoutpath);

	// FIXME: _this_ is hacky.
	dev = device_find_of_path(stdoutpath);
	if (!dev)
		goto remain;
	pri("Switch stdout to UART %s\n", dev->of.path);
	uart_stdout = dev->data;

	return 0;

remain:
	pri("Remaining on default console\n");
	return -ENOENT;
}
