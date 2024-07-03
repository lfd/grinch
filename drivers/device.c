/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "device: " x

#include <asm/spinlock.h>

#include <grinch/alloc.h>
#include <grinch/device.h>
#include <grinch/driver.h>
#include <grinch/ioremap.h>
#include <grinch/panic.h>
#include <grinch/symbols.h>

#define IDX_INVALID	(unsigned int)(-1)

static unsigned int dev_idx;

static DEFINE_SPINLOCK(devices_lock);
static LIST_HEAD(devices);

#define for_each_device(X)	list_for_each_entry((X), &devices, devices)

struct device *dev_find_of_path(const char *path)
{
	struct device *dev;

	spin_lock(&devices_lock);
	for_each_device(dev) {
		if (!dev->of.path)
			continue;

		if (!strcmp(path, dev->of.path))
			goto out;
	}
	dev = NULL;

out:
	spin_unlock(&devices_lock);
	return dev;
}

void __init dev_add(struct device *dev)
{
	if (dev->idx != IDX_INVALID)
		BUG();

	spin_lock(&devices_lock);
	dev->idx = dev_idx++;
	list_add(&dev->devices, &devices);
	spin_unlock(&devices_lock);
}

void __init dev_remove(struct device *dev)
{
	if (dev->idx == IDX_INVALID)
		return;

	spin_lock(&devices_lock);
	list_del(&dev->devices);
	spin_unlock(&devices_lock);

	dev->idx = IDX_INVALID;
}

void __init dev_init(struct device *dev, const char *name)
{
	INIT_LIST_HEAD(&dev->devices);
	dev->name = name;
	dev->idx = IDX_INVALID;
}

int __init dev_map_iomem(struct device *dev)
{
	int err;

	err = fdt_read_reg(_fdt, dev->of.node, 0, &dev->mmio.phys);
	if (err)
		return err;

	dev_pri(dev, "base: 0x%llx, size: 0x%lx\n",
	        (u64)dev->mmio.phys.paddr, dev->mmio.phys.size);

	err = ioremap_res(&dev->mmio);

	return err;
}

void dev_list(void)
{
	struct device *dev;

	spin_lock(&devices_lock);
	for_each_device(dev)
		pr("%u: %s\n", dev->idx, dev->name);
	spin_unlock(&devices_lock);
}
