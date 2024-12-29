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

#ifndef _GPAINT_H
#define _GPAINT_H

#include <grinch/gfb/gfb.h>

/* some pixel manipulating utilities */

void gfb_fill(struct gfb_handle *h, struct gcolor color);

#endif /* _GPAINT_H */
