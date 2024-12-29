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

#include <grinch/gfb/gpaint.h>

void gfb_fill(struct gfb_handle *h, struct gcolor color)
{
	struct gcoord coord;

	for (coord.y = 0; coord.y < h->gfb->info.mode.yres; coord.y++)
		for (coord.x = 0; coord.x < h->gfb->info.mode.xres; coord.x++)
			h->gfb->setpixel(h, coord, color);
}
