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

#ifndef _FS_H
#define _FS_H

#include <grinch/types.h>

#define MAX_PATHLEN	64

struct file;
struct file_system;

struct stat {
	loff_t size;
};

struct fs_flags {
	unsigned char may_read:1;
	unsigned char may_write:1;
	unsigned char is_kernel:1;
};

struct file_handle {
	struct file *fp;
	struct fs_flags flags;
	loff_t position;
};

struct file_operations {
	ssize_t (*read)(struct file_handle *, char *ubuf, size_t count);
	ssize_t (*write)(struct file_handle *, const char *ubuf, size_t count);
	void (*close)(struct file *);
	int (*stat)(struct file *, struct stat *st);
};

struct file {
	const struct file_operations *fops;
	void *drvdata;
};

struct file_system_operations {
	int (*open_file)(const struct file_system *fs, struct file *filep,
			 const char *fname, struct fs_flags flags);
};

struct file_system {
	const struct file_system_operations *fs_ops;
};

/* Routines */
int check_path(const char *path);
struct fs_flags get_flags(int oflag);

struct file *file_open(const char *path, struct fs_flags flags);
void file_close(struct file_handle *handle);

#endif /* _FS_H */
