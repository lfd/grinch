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
#include <grinch/time_abi.h>

#define S_IFMT 	00170000
#define S_IFREG	00100000
#define S_IFCHR 00020000
#define S_IFDIR	00040000
#define S_IFLNK	00120000

#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)

#define S_IRWXU	00700
#define S_IRUSR	00400
#define S_IWUSR	00200
#define S_IXUSR	00100

#define S_IRWXG	00070
#define S_IRGRP	00040
#define S_IWGRP	00020
#define S_IXGRP	00010

#define S_IRWXO	00007
#define S_IROTH	00004
#define S_IWOTH	00002
#define S_IXOTH	00001

struct stat {
	dev_t st_dev; // unused, for compat
	ino_t st_ino; // unused, for compat
	mode_t st_mode;
	nlink_t st_nlink; // unused, for compat
	uid_t st_uid; // unused, for compat
	gid_t st_gid; // unused, for compat
	dev_t st_rdev; // unused, for compat
	unsigned long long __pad;
	off_t st_size;
	blksize_t st_blksize; // unused, for compat
	int __pad2;
	blkcnt_t st_blocks; // unused, for compat
	struct timespec st_atim; // unused, for compat
	struct timespec st_mtim; // unused, for compat
	struct timespec st_ctim; // unused, for compat
	unsigned __unused[2];
};

#endif /* _GRINCH_STAT_H */
