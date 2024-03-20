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

enum devfs_type {
	DEVFS_REGULAR = 0, /* regular devfs node */
	DEVFS_SYMLINK, /* devfs symlink */
};

struct devfs_node {
	struct list_head nodes;

	char name[DEVFS_MAX_LEN_NAME];
	enum devfs_type type;

	const struct file_operations *fops;
	/*
	 * type == DEVFS_REGULAR -> drvdata points to data of driver
	 * type == DEVFS_SYMLINK -> drvdata points to destination node
	 */
	void *drvdata;
};

int devfs_init(void);

int __init devfs_register_node(struct devfs_node *node);
int devfs_create_symlink(const char *dst, const char *src);

/* /dev mountpoint */
extern struct file_system devfs;

#endif /* _DEVFS_H */
