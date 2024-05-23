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

#ifndef _FS_VFS_H
#define _FS_VFS_H

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
	unsigned char directory:1;
	unsigned char create:1;
};

struct file_handle {
	struct file *fp;
	struct fs_flags flags;
	loff_t position;
};

struct file_operations {
	/* File operations */
	ssize_t (*read)(struct file_handle *, char *ubuf, size_t count);
	ssize_t (*write)(struct file_handle *, const char *ubuf, size_t count);
	/* File + Directory operations */
	void (*close)(struct file *);

	int (*register_reader)(struct file_handle *h, char *ubuf, size_t count);
	int (*stat)(struct file *filep, struct stat *st);

	/* Directory operations */
	int (*getdents)(struct file_handle *h, struct grinch_dirent *udents,
			unsigned int size);
	int (*mkdir)(struct file *dir, struct file *filep, const char *name, mode_t mode);
	int (*create)(struct file *dir, struct file *filep, const char *name, mode_t mode);
};

struct file {
	const struct file_operations *fops;
	void *drvdata;
	bool is_directory;
};

struct file_system_operations {
	int (*open_file)(struct file *dir, struct file *filep,
			 const char *fname);
	int (*mount)(const struct file_system *fs, struct file *dir);
};

struct file_system {
	const struct file_system_operations *fs_ops;
	void *drvdata;
};


/* Opens a file, and increments references to the file */
struct file *file_open_at(struct file *at, const char *path);
static inline struct file *file_open(const char *path)
{
	return file_open_at(NULL, path);
}


struct file *file_ocreate_at(struct file *at, const char *path, bool create);

/* Releases the file */
void file_close(struct file *file);

/* Duplicates a file handle (e.g., used in fork()) */
void file_dup(struct file *file);

char *file_realpath(struct file *file);

/* VFS operations */
int vfs_init(void);

void *vfs_read_file(const char *pathname, size_t *len);
int vfs_stat_at(struct file *at, const char *pathname, struct stat *st);

int vfs_mkdir_at(struct file *at, const char *pathname, mode_t mode);
static inline int vfs_mkdir(const char *pathname, mode_t mode)
{
	return vfs_mkdir_at(NULL, pathname, mode);
}

void vfs_lsof(void);

#endif /* _FS_VFS_H */
