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

#ifndef _GRINCH_FB_ABI_H
#define _GRINCH_FB_ABI_H

#include <grinch/types.h>

/* Framebuffer ioctls */
#define GRINCH_FB_SCREENINFO	0
#define GRINCH_FB_MODESET	1

struct grinch_fb_modeinfo {
	u32 xres;
	u32 yres;
	u8 bpp;
};

struct grinch_fb_screeninfo {
	struct grinch_fb_modeinfo mode;
	u32 fb_size; /* size of fb in current resolution */
	u32 mmio_size; /* size of full mmio-mapped frame buffer */
};

#endif /* _GRINCH_FB_ABI_H */
