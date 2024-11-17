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

#ifndef _GFB_H
#define _GFB_H

#include <grinch/fb_abi.h>

struct gcolor {
	u8 r;
	u8 g;
	u8 b;
};

struct gcoord {
	unsigned int x;
	unsigned int y;
};

struct gfb_handle {
	struct gfb *gfb;
	void *fb;
};

typedef void (*g_setpixel)(struct gfb_handle *h,
			   struct gcoord coord, struct gcolor color);

struct gfb {
	int fd;
	struct gfb_screeninfo info;
	g_setpixel setpixel;
};

int gfb_open(struct gfb *fb, const char *dev);
void gfb_close(struct gfb *fb);

static inline bool gfb_pixmode_supported(struct gfb *fb, pixmode_t mode)
{
	return fb->info.pixmodes_supported & (1UL << mode);
}

void gfb_modeinfo(struct gfb *fb);
int gfb_modeset(struct gfb *fb, struct gfb_mode *mode);

#endif /* _GFB_H */
