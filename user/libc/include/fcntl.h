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

#ifndef _FCNTL_H
#define _FCNTL_H

int open(const char *file, int oflag);

#include <asm-generic/fcntl.h>

#endif /* _FCNTL_H */
