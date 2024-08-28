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

#include <grinch/gpaint.h>

static void setpixel_xrgb(void *fb, struct grinch_fb_screeninfo *info,
			  struct gcoord coord, struct gcolor color)
{
	u32 *dst;

	dst = fb + (32 / 8) * (coord.y * info->mode.xres + coord.x);

	*dst = (color.r << 16) |
	       (color.g << 8) |
	       (color.b << 0);
}

static void setpixel_rgb(void *fb, struct grinch_fb_screeninfo *info,
			  struct gcoord coord, struct gcolor color)
{
	u8 *dst;

	dst = fb + (24 / 8) * (coord.y * info->mode.xres + coord.x);

	dst[0] = color.b;
	dst[1] = color.g;
	dst[2] = color.r;
}

static void setpixel_rbg(void *fb, struct grinch_fb_screeninfo *info,
			  struct gcoord coord, struct gcolor color)
{
	u8 *dst;

	dst = fb + (24 / 8) * (coord.y * info->mode.xres + coord.x);

	dst[0] = color.g;
	dst[1] = color.b;
	dst[2] = color.r;
}

g_setpixel g_get_setpixel(struct grinch_fb_screeninfo *info)
{
	switch (info->mode.pixmode) {
		case GRINCH_FB_PIXMODE_XRGB:
			return setpixel_xrgb;

		case GRINCH_FB_PIXMODE_RGB:
			return setpixel_rgb;

		case GRINCH_FB_PIXMODE_RBG:
			return setpixel_rbg;

		default:
			return NULL;
	}
}
