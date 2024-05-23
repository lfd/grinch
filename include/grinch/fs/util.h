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

#ifndef _FS_UTIL_H
#define _FS_UTIL_H

#include <grinch/dirent.h>

char *pathname_from_user(const char __user *pathname, bool *must_dir);
int copy_dirent(struct grinch_dirent __mayuser *udent, bool is_kernel,
		struct grinch_dirent *src, const char *name,
		unsigned int size);

#endif /* _FS_UTIL */
