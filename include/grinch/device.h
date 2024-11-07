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

#ifndef _DEVICE_H
#define _DEVICE_H

#include <grinch/list.h>
#include <grinch/iores.h>

#define _dev_prr(dev, header, fmt, ...)		_prr(header, "%s: " fmt, (dev)->name, ##__VA_ARGS__)
#define _dev_prri(dev, header, fmt, ...)	_prri(header, "%s: " fmt, (dev)->name, ##__VA_ARGS__)

#define dev_pr(dev, fmt, ...)	_dev_prr(dev, PR_DEFAULT, fmt, ##__VA_ARGS__)
#define dev_pri(dev, fmt, ...)	_dev_prri(dev, PR_DEFAULT, fmt, ##__VA_ARGS__)

#define dev_pr_crit(dev, fmt, ...) _dev_prr(dev, PR_CRIT, fmt, ##__VA_ARGS__)
#define dev_pri_crit(dev, fmt, ...) _dev_prri(dev, PR_CRIT, fmt, ##__VA_ARGS__)

#define dev_pr_warn(dev, fmt, ...) _dev_prr(dev, PR_WARN, fmt, ##__VA_ARGS__)
#define dev_pri_warn(dev, fmt, ...) _dev_prri(dev, PR_WARN, fmt, ##__VA_ARGS__)

#define dev_pr_info(dev, fmt, ...) _dev_prr(dev, PR_INFO, fmt, ##__VA_ARGS__)
#define dev_pri_info(dev, fmt, ...) _dev_prri(dev, PR_INFO, fmt, ##__VA_ARGS__)

#define dev_pr_dbg(dev, fmt, ...) _dev_prr(dev, PR_DEBUG, fmt, ##__VA_ARGS__)
#define dev_pri_dbg(dev, fmt, ...) _dev_prri(dev, PR_DEBUG, fmt, ##__VA_ARGS__)

struct device {
	unsigned int idx;
	const char *name;

	struct list_head devices;
	struct {
		const char *path;
		int node;
		const struct of_device_id *match;
	} of;
	struct mmio_resource mmio;

	void *data;
};

/* Device helper routines */
struct device *dev_find_of_path(const char *path);

// FIXME: Devices may have multiple MMIO resources
int _dev_map_iomem(struct device *dev, size_t szmax);
static inline int dev_map_iomem(struct device *dev)
{
	return _dev_map_iomem(dev, -1);
}

void dev_init(struct device *dev, const char *name);
void dev_add(struct device *dev);
void dev_remove(struct device *dev);

void dev_list(void);

#endif /* _DEVICE_H */
