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

#ifndef _DRIVER_H
#define _DRIVER_H

#include <grinch/fdt.h>
#include <grinch/list.h>
#include <grinch/printk.h>

struct device;

enum driver_prio {
	PRIO_0 = 0, /* Highest */
	PRIO_1, /* Lowest */
	PRIO_MAX
};

struct driver {
	const char *name;
	enum driver_prio prio;
	int (*init)(void);
	struct {
		int (*probe)(struct device *dev);
		const struct of_device_id *matches;
	} of;
};

#define DECLARE_DRIVER(ID, NAME, PRIO, INIT, PROBE, MATCHES)	\
static const struct driver 					\
__driver_##ID __used __section(".drivers") = {			\
	.name = NAME,						\
	.prio = PRIO,						\
	.init = INIT,						\
	.of.probe = PROBE,					\
	.of.matches = MATCHES,					\
};

int driver_init(void);

#endif /* _DRIVER_H */
