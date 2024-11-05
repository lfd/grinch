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

#ifndef _IOCTL_H
#define _IOCTL_H

#include <grinch/stat.h>

int ioctl(int fd, int op, ...);

#endif /* _IOCTL_H */
