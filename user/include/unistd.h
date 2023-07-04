/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>

typedef int pid_t;

ssize_t write(int fd, const void *buf, size_t count);
pid_t getpid(void);

#endif /* _UNISTD_H */
