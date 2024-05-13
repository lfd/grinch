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

#ifndef _FS_FS_H
#define _FS_FS_H

#include <grinch/dirent.h>
#include <grinch/types.h>
#include <grinch/stat.h>

#define MAX_PATHLEN	64

struct file;
struct file_system;

struct fs_flags {
	unsigned char may_read:1;
	unsigned char may_write:1;
	unsigned char nonblock:1;
	unsigned char is_kernel:1;
	unsigned char must_directory:1;
	unsigned char create:1;
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
	int (*register_reader)(struct file_handle *h, char *ubuf, size_t count);
	int (*getdents)(struct file_handle *h, struct grinch_dirent *udents,
			unsigned int size);
};

struct file {
	const struct file_operations *fops;
	void *drvdata;
};

struct file_system_operations {
	int (*open_file)(const struct file_system *fs, struct file *filep,
			 const char *fname, struct fs_flags flags);
	int (*stat)(const struct file_system *fs, const char *pathname,
		    struct stat *st);
	int (*mkdir)(const struct file_system *fs, const char *pathname,
		     mode_t mode);
};

struct file_system {
	const struct file_system_operations *fs_ops;
};

/* Routines */
struct file *file_open(const char *path, struct fs_flags flags);
void file_close(struct file_handle *handle);

void file_get(struct file *file);

#endif /* _FS_FS_H */
