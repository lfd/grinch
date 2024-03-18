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
#include <grinch/devfs.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/percpu.h>
#include <grinch/irqchip.h>
#include <grinch/mmio.h>
#include <grinch/printk.h>
#include <grinch/serial.h>
#include <grinch/vsprintf.h>

#include <grinch/fs.h>

#define SERIAL_RINGBUF_SIZE	32

static unsigned int uart_no;

void serial_in(struct uart_chip *c, char ch)
{
	spin_lock(&c->lock);
	ringbuf_write(&c->rb, ch);
	spin_unlock(&c->lock);
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
	struct devfs_node *node;
	struct uart_chip *c;
	int err;

	err = uart_init(dev);
	if (err)
		return err;

	d = dev->of.match->data;

	c = dev->data;
	c->driver = d;
	err = d->init(c);
	if (err)
		goto error_out;

	err = ringbuf_init(&c->rb, SERIAL_RINGBUF_SIZE);
	if (err)
		goto error_out;

	node = &c->node;
	snprintf(node->name, sizeof(node->name), ISTR("ttyS%u"), uart_no);
	node->fops = &serial_fops;
	node->drvdata = dev;
	err = devfs_register_node(node);
	if (err)
		goto error_out;

	pri("Registered as %s\n", node->name);
	uart_no++;

	return 0;

error_out:
	uart_deinit(dev);
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

	ringbuf_free(&c->rb);

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
