/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"devfs: " x

#include <string.h>

#include <asm/spinlock.h>

#include <grinch/alloc.h>
#include <grinch/devfs.h>
#include <grinch/errno.h>
#include <grinch/kstr.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

static LIST_HEAD(devfs_nodes);
static DEFINE_SPINLOCK(devfs_lock);

static ssize_t dev_zero_write(struct file_handle *, const char *, size_t count)
{
	return count;
}

static ssize_t dev_zero_read(struct file_handle *h, char *ubuf, size_t count)
{
	if (h->flags.is_kernel) {
		memset(ubuf, 0, count);
		return count;
	} else
		return umemset(&current_process()->mm, ubuf, 0, count);
}

static ssize_t dev_null_read(struct file_handle *, char *, size_t)
{
	return 0;
}

static const struct file_operations dev_zero_fops = {
	.read = dev_zero_read,
	.write = dev_zero_write,
};

static const struct file_operations dev_null_fops = {
	.read = dev_null_read,
	.write = dev_zero_write,
};

static int devfs_open(const struct file_system *fs, struct file *filep,
		      const char *path, struct fs_flags flags)
{
	struct devfs_node *node;
	int err;

	spin_lock(&devfs_lock);

	list_for_each_entry(node, &devfs_nodes, nodes)
		if (!strcmp(node->name, path))
			goto found;
	err = -ENOENT;
	goto unlock_out;

found:
	if (node->type == DEVFS_SYMLINK)
		node = node->drvdata;

	if ((flags.may_read && !node->fops->read) ||
	    (flags.may_write && !node->fops->write)) {
		err = -EBADF;
		goto unlock_out;
	}

	filep->fops = node->fops;
	filep->drvdata = node->drvdata;
	err = 0;

unlock_out:
	spin_unlock(&devfs_lock);
	return err;
}

int __init devfs_create_symlink(const char *dst, const char *src)
{
	struct devfs_node *node, *src_node;
	int err;

	src_node = NULL;

	spin_lock(&devfs_lock);
	list_for_each_entry(node, &devfs_nodes, nodes) {
		if (!strcmp(node->name, dst)) {
			err = -EEXIST;
			goto unlock_out;
		}
		if (!strcmp(node->name, src))
			src_node = node;
	}
	if (!src_node) {
		err = -ENOENT;
		goto unlock_out;
	}

	node = kzalloc(sizeof(*node));
	if (!node) {
		err = -ENOMEM;
		goto unlock_out;
	}

	node->type = DEVFS_SYMLINK;
	strncpy(node->name, dst, sizeof(node->name) - 1);
	node->drvdata = src_node;
	list_add(&node->nodes, &devfs_nodes);

	err = 0;

unlock_out:
	spin_unlock(&devfs_lock);
	return err;
}

int __init devfs_register_node(struct devfs_node *node)
{
	struct devfs_node *tmp;
	int err;

	spin_lock(&devfs_lock);

	list_for_each_entry(tmp, &devfs_nodes, nodes)
		if (!strcmp(node->name, tmp->name)) {
			err = -EEXIST;
			goto unlock_out;
		}

	list_add(&node->nodes, &devfs_nodes);
	err = 0;

unlock_out:
	spin_unlock(&devfs_lock);
	return err;
}

static const struct file_system_operations fs_ops_devfs = {
	.open_file = devfs_open,
};

/* This is the /dev "mount point" */
struct file_system devfs = {
	.fs_ops = &fs_ops_devfs,
};

static struct devfs_node devfs_constants[] = {
	{
		.name = "zero",
		.type = DEVFS_REGULAR,
		.fops = &dev_zero_fops,
	},
	{
		.name = "null",
		.type = DEVFS_REGULAR,
		.fops = &dev_null_fops,
	},
};

int __init devfs_init(void)
{
	int err;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(devfs_constants); i++) {
		err = devfs_register_node(&devfs_constants[i]);
		if (err)
			return err;
	}

	return 0;
}
