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

#define GRINCH_FB_PIXMODE_XRGB		0 // 32-Bit
#define GRINCH_FB_PIXMODE_RGB		1 // 24-Bit
#define GRINCH_FB_PIXMODE_RBG		2 // 24-Bit
#define GRINCH_FB_PIXMODE_R5G6B5	3 // 16-Bit mode R5G6B5 - Bochs
#define GRINCH_FB_PIXMODE_R5G5B5	4 // 15 (16)-Bit mode R5G5B5 - Bochs

typedef u8 pixmode_t;

struct grinch_fb_modeinfo {
	u32 xres;
	u32 yres;
	pixmode_t pixmode;
};

struct grinch_fb_screeninfo {
	struct grinch_fb_modeinfo mode;

	/* size of one frame of the framebuffer in the current mode */
	u32 fb_size;
	u32 bpp;

	u64 pixmodes_supported;
};

#endif /* _GRINCH_FB_ABI_H */
