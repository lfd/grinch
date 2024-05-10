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

#include <grinch/stat.h>

void *vfs_read_file(const char *pathname, size_t *len);
int vfs_stat(const char *pathname, struct stat *st);
int vfs_mkdir(const char *pathname, mode_t mode);

int vfs_init(void);

void vfs_lsof(void);

#endif /* _FS_VFS_H */
