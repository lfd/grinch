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

#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/init.h>
#include <grinch/irqchip.h>
#include <grinch/printk.h>
#include <grinch/symbols.h>

/* Maximum number of cascaded interrupt controllers in the system */
#define IRQCHIP_MAX_CHIPS	4

const struct irqchip_fn *irqchip_fn;

static irq_handler_t irq_handlers[IRQ_MAX];
static void *irq_handlers_userdata[IRQ_MAX];

struct irqchip_candidate {
	const struct driver *drv;
	int node;
	int parent;
	bool probed;
};

#define for_each_irqchip_driver(X)					\
	for ((X) = (const struct driver *)__irqchip_drivers_start;	\
	     (X) < (const struct driver *)__irqchip_drivers_end;	\
	     (X)++)

static int __init irqchip_parent_node(int node)
{
	const fdt32_t *prop;
	int off, len;

	prop = fdt_getprop(_fdt, node, ISTR("interrupts-extended"), &len);
	if (prop && len >= (int)sizeof(fdt32_t))
		return fdt_node_offset_by_phandle(_fdt, fdt32_to_cpu(prop[0]));

	/* interrupt-parent may be inherited from ancestor nodes */
	for (off = node; off >= 0; off = fdt_parent_offset(_fdt, off)) {
		prop = fdt_getprop(_fdt, off, ISTR("interrupt-parent"), &len);
		if (prop && len == sizeof(fdt32_t))
			return fdt_node_offset_by_phandle(_fdt,
							  fdt32_to_cpu(prop[0]));
	}

	return -ENOENT;
}

int __init irqchip_init(void)
{
	struct irqchip_candidate cand[IRQCHIP_MAX_CHIPS], *chip, *other;
	const struct driver *drv;
	bool progress, defer;
	int off, sub, err;
	unsigned int n_chips;

	off = fdt_path_offset(_fdt, ISTR("/soc"));
	if (off < 0)
		off = 0;

	/* Collect all interrupt-controller nodes with a matching driver */
	n_chips = 0;
	fdt_for_each_subnode(sub, _fdt, off) {
		if (!fdt_getprop(_fdt, sub, ISTR("interrupt-controller"),
				 NULL))
			continue;

		for_each_irqchip_driver(drv) {
			if (!drv->of.matches)
				continue;

			if (fdt_match_device_off(_fdt, sub, drv->of.matches,
						 NULL) <= 0)
				continue;

			if (n_chips == IRQCHIP_MAX_CHIPS)
				return -E2BIG;

			chip = &cand[n_chips];
			chip->drv = drv;
			chip->node = sub;
			chip->parent = irqchip_parent_node(sub);
			chip->probed = false;
			n_chips++;
			break;
		}
	}

	if (!n_chips)
		return -ENOENT;

	/* Probe root controllers first, cascaded controllers afterwards */
	do {
		progress = false;
		for (chip = cand; chip < &cand[n_chips]; chip++) {
			if (chip->probed)
				continue;

			/* Defer while this chip's parent is not up yet */
			defer = false;
			for (other = cand; other < &cand[n_chips]; other++) {
				if (other != chip && !other->probed &&
				    other->node == chip->parent)
					defer = true;
			}
			if (defer)
				continue;

			err = driver_probe_node(chip->drv, chip->node);
			if (err == -ENOMEM)
				return err;

			chip->probed = true;
			progress = true;
		}
	} while (progress);

	return 0;
}

int irq_register_handler(u32 irq, irq_handler_t handler, void *userdata)
{
	if (!handler)
		return -EINVAL;

	if (irq >= IRQ_MAX)
		return -EINVAL;

	irq_handlers[irq] = handler;
	irq_handlers_userdata[irq] = userdata;

	return 0;
}

void irqchip_handle_irq(unsigned int irq)
{
	irq_handler_t handler = NULL;
	int err;

	if (irq < IRQ_MAX)
		handler = irq_handlers[irq];

	if (handler) {
		err = handler(irq_handlers_userdata[irq]);
		if (err)
			pr("Handler error for IRQ %u: %pe\n",
			   irq, ERR_PTR(err));
	} else
		pr("No Handler for IRQ %u\n", irq);
}
