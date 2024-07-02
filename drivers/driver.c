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
#include <grinch/device.h>
#include <grinch/driver.h>
#include <grinch/ioremap.h>
#include <grinch/symbols.h>

#define for_each_driver(X)					\
	for ((X) = (const struct driver *)__drivers_start;	\
	     (X) < (const struct driver *)__drivers_end;	\
	     (X)++)

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
			dev = dev_create(drv->name);
			if (IS_ERR(dev))
				return PTR_ERR(dev);

			dev->of.path = kstrdup(path);
			dev->of.node = sub;
			dev->of.match = match;

			err = drv->of.probe(dev);
			if (err) {
				pri("Driver %s failed probing %s: %pe\n",
				    drv->name, path, ERR_PTR(err));
				dev_destroy(dev);
				continue;
			}

			dev_add(dev);
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
