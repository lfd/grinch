/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* LiteX simulation finish: a write ends the simulator process. */

#include <grinch/device.h>
#include <grinch/driver.h>
#include <grinch/errno.h>
#include <grinch/mmio.h>
#include <grinch/reboot.h>

static void *finish_reg;

static int sim_finish_shutdown(int err)
{
	mmio_write32(finish_reg, 1);
	return -EIO;
}

static int __init sim_finish_probe(struct device *dev)
{
	int err;

	err = dev_map_iomem(dev);
	if (err)
		return err;

	finish_reg = dev->mmio.base;
	shutdown_method_set(sim_finish_shutdown, REBOOT_PRIO_PLATFORM);

	return 0;
}

static const struct of_device_id sim_finish_matches[] = {
	{ .compatible = "litex,sim-finish", },
	{ /* Sentinel */ }
};

DECLARE_DRIVER(sim_finish, "sim-finish", PRIO_1, NULL, sim_finish_probe,
	       sim_finish_matches);
