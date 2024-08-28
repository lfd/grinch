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

#include <grinch/fb.h>
#include <grinch/types.h>

struct gcolor {
	u8 r;
	u8 g;
	u8 b;
};

struct gcoord {
	unsigned int x;
	unsigned int y;
};

typedef void (*g_setpixel)(void *fb, struct grinch_fb_screeninfo *info,
			   struct gcoord coord, struct gcolor color);

g_setpixel g_get_setpixel(struct grinch_fb_screeninfo *info);

#endif /* _GPAINT_H */
