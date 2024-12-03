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

#ifndef _GRINCH_DIRENT_H
#define _GRINCH_DIRENT_H

#include <grinch/types.h>

#define DT_UNKNOWN	0
#define DT_REG		1
#define DT_DIR		2
#define DT_LNK		3

struct dirent {
	ino_t d_ino; // unused (for compat)
	off_t d_off; // unused (for compat)
	unsigned short d_reclen; // unused (for compat)
	unsigned char d_type;
	char d_name[];
};

#endif /* _GRINCH_DIRENT_H */
