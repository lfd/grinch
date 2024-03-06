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

#ifndef _DEVFS_H
#define _DEVFS_H

#include <grinch/fs.h>
#include <grinch/init.h>
#include <grinch/list.h>

#define DEVFS_MAX_LEN_NAME	16
#define DEVFS_MOUNTPOINT	"/dev/"
#define DEVFS_MOUNTPOINT_LEN	(sizeof(DEVFS_MOUNTPOINT) - 1)

struct devfs_node {
	struct list_head nodes;

	char name[DEVFS_MAX_LEN_NAME];
	const struct file_operations *fops;
	void *drvdata;
};

int devfs_init(void);

int __init devfs_register_node(struct devfs_node *node);

/* /dev mountpoint */
extern struct file_system devfs;

#endif /* _DEVFS_H */
