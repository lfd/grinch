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

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <grinch/init.h>

extern const char *platform_model;

int __init platform_init(void);

#endif /* _PLATFORM_H */
