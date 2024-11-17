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

#ifndef _ERRNO_H
#define _ERRNO_H

#include <grinch/errno.h>

/* FIXME: We don't have thread-local storage at the moment */
extern int errno;

#endif /* _ERRNO_H */
