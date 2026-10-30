/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/atomic.h>
#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/init.h>
#include <grinch/irqchip.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/smp.h>
#include <grinch/spinlock.h>
#include <grinch/symbols.h>

/* Maximum number of cascaded interrupt controllers in the system */
#define IRQCHIP_MAX_CHIPS	4

const struct irqchip_fn *irqchip_fn;

static struct irq_action *irq_actions[IRQ_MAX];

/*
 * Serializes the list mutators against each other. The handler walk runs
 * lock-free: an append publishes with a release so a concurrent walk sees a
 * fully initialised action, and an unregister requires the line to be quiet
 * (disabled, or its device not yet live) -- there is no synchronize_irq.
 */
static DEFINE_SPINLOCK(irq_actions_lock);

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

int irqchip_decode(const u32 *cells, unsigned int n)
{
	if (!irqchip_fn || !irqchip_fn->decode)
		return -ENOSYS;

	return irqchip_fn->decode(cells, n);
}

int irqchip_enable_irq(unsigned int irq)
{
	if (irqchip_fn)
		return irqchip_fn->enable_irq(irq);

	return -ENOENT;
}

int irqchip_set_affinity(unsigned int irq, unsigned long cpu)
{
	/* Routing to an offline CPU would mask the source everywhere but an
	 * interface that never services it. */
	if (!cpu_is_online(cpu))
		return -EINVAL;

	if (irqchip_fn && irqchip_fn->set_affinity)
		return irqchip_fn->set_affinity(irq, cpu);

	return -ENOSYS;
}

void irqchip_send_ipi(unsigned long cpuid)
{
	if (irqchip_fn && irqchip_fn->send_ipi)
		irqchip_fn->send_ipi(cpuid);
}

int irqchip_cpu_init(void)
{
	/* Local per-hart interrupt bring-up (e.g. the riscv IPI). */
	arch_irqchip_cpu_init();

	/* External interrupt controller per-CPU setup, if one is present. */
	if (irqchip_fn && irqchip_fn->cpu_init)
		return irqchip_fn->cpu_init();

	return 0;
}

int irq_register_action(u32 irq, struct irq_action *act)
{
	struct irq_action *head, **tail;
	unsigned long flags;
	int ret;

	if (!act || !act->handler)
		return -EINVAL;

	if (irq >= IRQ_MAX)
		return -EINVAL;

	flags = spin_lock_irqsave(&irq_actions_lock);

	ret = -EBUSY;
	head = irq_actions[irq];
	if (head && !((head->flags & act->flags) & IRQ_SHARED))
		goto unlock_out;

	tail = &irq_actions[irq];
	while (*tail) {
		/* Registering an action twice would loop the list on itself. */
		if (*tail == act)
			BUG();
		tail = &(*tail)->next;
	}

	/* Publish only a fully initialised action to the lock-free walk. */
	act->next = NULL;
	smp_store_release(tail, act);
	ret = 0;

unlock_out:
	spin_unlock_irqrestore(&irq_actions_lock, flags);

	return ret;
}

void irq_unregister_action(u32 irq, struct irq_action *act)
{
	struct irq_action **link;
	unsigned long flags;

	if (irq >= IRQ_MAX)
		return;

	flags = spin_lock_irqsave(&irq_actions_lock);
	for (link = &irq_actions[irq]; *link; link = &(*link)->next)
		if (*link == act) {
			WRITE_ONCE(*link, act->next);
			act->next = NULL;
			break;
		}
	spin_unlock_irqrestore(&irq_actions_lock, flags);
}

void irqchip_handle_irq(unsigned int irq)
{
	struct irq_action *act;
	int ret = IRQ_NONE;

	if (irq >= IRQ_MAX || !irq_actions[irq]) {
		pr("No Handler for IRQ %u\n", irq);
		return;
	}

	for (act = READ_ONCE(irq_actions[irq]); act; act = READ_ONCE(act->next)) {
		int res = act->handler(act->userdata);

		if (res < 0) {
			pr("Handler error for IRQ %u: %pe\n", irq, ERR_PTR(res));
			ret |= IRQ_HANDLED;
		} else
			ret |= res;
	}

	if (ret == IRQ_NONE)
		pr("Unclaimed IRQ %u\n", irq);
}
