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

#ifndef _GRINCH_GRINCH_H
#define _GRINCH_GRINCH_H

#include <grinch/gcall.h>

int gcall(unsigned long no, unsigned long arg1);

#endif /* _GRINCH_GRINCH_H */
