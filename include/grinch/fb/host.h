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

#include <grinch/device.h>
#include <grinch/fb_abi.h>
#include <grinch/fs/devfs.h>

struct fb_host {
	struct devfs_node node;
	struct grinch_fb_screeninfo info;
	struct device *dev;

	spinlock_t lock;

	int (*set_mode)(struct fb_host *host, struct grinch_fb_modeinfo *mode);

	// FIXME: we should support multiple framebuffers
	void *fb;

	unsigned long private[];
};

static inline void *fb_priv(struct fb_host *host)
{
	return (void *)host->private;
}

struct fb_host *fb_host_alloc(unsigned int sz, struct device *dev);
void fb_host_dealloc(struct fb_host *host);

int fb_host_add(struct fb_host *host);
void fb_host_remove(struct fb_host *host);
