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

#ifndef _DIRENT_H
#define _DIRENT_H

#include <grinch/dirent.h>

int getdents(int fd, struct grinch_dirent *dents, unsigned int size);

#endif /* _DIRENT_H */
