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
#include <grinch/ringbuf.h>

#define DEVFS_MAX_LEN_NAME	16
#define DEVFS_MOUNTPOINT	"/dev/"
#define DEVFS_MOUNTPOINT_LEN	(sizeof(DEVFS_MOUNTPOINT) - 1)

#define DEVICE_NAME(X)		DEVFS_MOUNTPOINT X

enum devfs_type {
	DEVFS_REGULAR = 0, /* regular devfs node */
	DEVFS_CHARDEV, /* character device with a ringbuffer */
	DEVFS_SYMLINK, /* devfs symlink */
};

struct devfs_node {
	struct list_head nodes;

	char name[DEVFS_MAX_LEN_NAME];
	enum devfs_type type;
	spinlock_t lock;

	const struct file_operations *fops;
	/*
	 * type == DEVFS_REGULAR -> drvdata points to data of driver
	 * type == DEVFS_SYMLINK -> drvdata points to destination node
	 */
	void *drvdata;

	struct ringbuf rb;
};

int devfs_init(void);

int devfs_node_register(struct devfs_node *node);
void devfs_node_unregister(struct devfs_node *node);
int devfs_create_symlink(const char *dst, const char *src);

int devfs_node_init(struct devfs_node *node);
void devfs_node_deinit(struct devfs_node *node);

/* /dev mountpoint */
extern struct file_system devfs;

void devfs_chardev_write(struct devfs_node *node, char c);
ssize_t devfs_chardev_read(struct devfs_node *node, struct file_handle *h, char *buf, size_t count);

#endif /* _DEVFS_H */
