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

#define dbg_fmt(x) "driver: " x

#include <asm/spinlock.h>

#include <grinch/alloc.h>
#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/printk.h>
#include <grinch/kstr.h>
#include <grinch/init.h>
#include <grinch/symbols.h>

#define for_each_driver(X)					\
	for ((X) = (const struct driver *)__drivers_start;	\
	     (X) < (const struct driver *)__drivers_end;	\
	     (X)++)

DEFINE_SPINLOCK(devices_lock);
LIST_HEAD(devices);

#define for_each_device(X)	list_for_each_entry((X), &devices, devices)

struct device *device_find_of_path(const char *path)
{
	struct device *dev;

	spin_lock(&devices_lock);
	for_each_device(dev) {
		if (!strcmp(path, dev->of.path))
			goto out;
	}
	dev = NULL;

out:
	spin_unlock(&devices_lock);
	return dev;
}

static int __init drivers_probe(enum driver_prio prio)
{
	const struct of_device_id *match;
	const struct driver *drv;
	struct device *dev;
	int err, off, sub;
	char path[64];

	off = fdt_path_offset(_fdt, ISTR("/soc"));
	if (off <= 0)
		return -ENOENT;

	fdt_for_each_subnode(sub, _fdt, off) {
		err = fdt_get_path(_fdt, sub, path, sizeof(path));
		
		for_each_driver(drv) {
			if (drv->prio != prio)
				continue;

			if (!drv->of.matches)
				continue;

			err = fdt_match_device_off(_fdt, sub, drv->of.matches,
						   &match);
			if (err <= 0)
				continue;

			/* We have a match */
			err = fdt_get_path(_fdt, sub, path, sizeof(path));
			if (err) {
				pri("%s: Error getting path name: %d\n",
				    drv->name, err);
				continue;
			}

			pri("%s: Initialising device %s\n", drv->name, path);
			dev = kzalloc(sizeof(*dev));
			if (!dev)
				return -ENOMEM;

			dev->of.path = kstrdup(path);
			dev->of.node = sub;
			dev->of.match = match;

			err = drv->of.probe(dev);
			if (err) {
				pri("Driver %s failed probing %s: %pe\n",
				    drv->name, path, ERR_PTR(err));
				kfree(dev->of.path);
				kfree(dev);
				continue;
			}
			spin_lock(&devices_lock);
			list_add(&dev->devices, &devices);
			spin_unlock(&devices_lock);
		}
	}

	return 0;
}

int __init driver_init(void)
{
	const struct driver *drv;
	enum driver_prio prio;
	int err;

	for_each_driver(drv) {
		if (!drv->init)
			continue;

		err = drv->init();
		if (err)
			pr_warn_i("Error initialising driver %s\n", drv->name);
	}

	for (prio = PRIO_0; prio < PRIO_MAX; prio++) {
		err = drivers_probe(prio);
		if (err)
			return err;
	}

	return 0;
}
