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

#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/minmax.h>
#include <grinch/vsprintf.h>

#include <grinch/fb/host.h>

static __initdata unsigned int fb_index;
static __initdata DEFINE_SPINLOCK(fb_lock);

static unsigned int __init fb_get_next_index(void)
{
	unsigned int ret;

	spin_lock(&fb_lock);
	ret = fb_index++;
	spin_unlock(&fb_lock);

	return ret;
}

struct fb_host * __init fb_host_alloc(unsigned int sz, struct device *dev)
{
	struct fb_host *host;


	host = kzalloc(sizeof(*host) + sz);
	if (!host)
		return NULL;

	spin_init(&host->lock);
	host->dev = dev;

	return host;
}

static long
fb_host_ioctl(struct devfs_node *node, unsigned long op, unsigned long arg)
{
	struct grinch_fb_modeinfo mode;
	struct fb_host *host;
	void __user *uarg;
	unsigned long ret;
	long err;

	host = node->drvdata;
	uarg = (void __user *)arg;

	spin_lock(&host->lock);

	switch (op) {
		case GRINCH_FB_SCREENINFO:
			ret = copy_to_user(current_task(), uarg, &host->info,
					   sizeof(host->info));
			if (ret != sizeof(host->info))
				return -EFAULT;

			err = 0;
			break;

		case GRINCH_FB_MODESET:
			ret = copy_from_user(current_task(), &mode, uarg,
					     sizeof(mode));
			if (ret != sizeof(mode))
				return -EFAULT;

			err = host->set_mode(host, &mode);
			break;

		default:
			err = -EINVAL;
	}

	spin_unlock(&host->lock);

	return err;
}

static ssize_t fb_host_write(struct devfs_node *node, struct file_handle *fh,
			     const char *ubuf, size_t count)
{
	struct fb_host *host;

	host = node->drvdata;
	spin_lock(&host->lock);

	if (count != host->info.fb_size) {
		dev_pr_warn(host->dev, "Input size %lx: too %s\n", count,
			    count < host->info.fb_size ? "small" : "large");
		count = min(count, host->info.fb_size);
	}

	if (fh->flags.is_kernel)
		memcpy(host->fb, ubuf, count);
	else
		copy_from_user(current_task(), host->fb, ubuf, count);

	spin_unlock(&host->lock);

	return count;
}

static const struct devfs_ops fb_host_fops = {
	.write = fb_host_write,
	.ioctl = fb_host_ioctl,
};

int __init fb_host_add(struct fb_host *host)
{
	int err;

	snprintf(host->node.name, sizeof(host->node.name), ISTR("fb%u"),
		 fb_get_next_index());
	host->node.type = DEVFS_REGULAR;
	host->node.fops = &fb_host_fops;
	host->node.drvdata = host;
	err = devfs_node_init(&host->node);
	if (err)
		return err;

	err = devfs_node_register(&host->node);
	if (err) {
		devfs_node_deinit(&host->node);
		return err;
	}

	return 0;
}

void __init fb_host_dealloc(struct fb_host *host)
{
	kfree(host);
}

void __init fb_host_remove(struct fb_host *host)
{
	devfs_node_unregister(&host->node);
	devfs_node_deinit(&host->node);
}
