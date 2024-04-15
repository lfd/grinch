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

#ifndef _GRINCH_STAT_H
#define _GRINCH_STAT_H

#include <grinch/types.h>

#define S_IFMT 	00170000
#define S_IFREG	00100000
#define S_IFCHR 00020000
#define S_IFDIR	00040000
#define S_IFLNK	00120000

#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)

struct stat {
	unsigned short st_mode;
	loff_t st_size;
};

#endif /* _GRINCH_STAT_H */
