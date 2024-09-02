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

/* Font definitions (GPLv2) are copied and converted from the Linux kernel */

#ifndef _GFB_FONT_H
#define _GFB_FONT_H

#include <grinch/compiler_attributes.h>
#include <grinch/gfb/gfb.h>

struct gfont {
	unsigned int width, height;
	unsigned int charcount;
	const char *name;
	const unsigned short data[];
} __packed;

extern const struct gfont
	font_6x8,
	font_6x10,
	font_7x14,
	font_10x18,
	font_acorn_8x8,
	font_mini_4x6,
	font_pearl_8x8,
	font_sun_12x22,
	font_sun_8x16,
	font_ter_16x32,
	font_vga_6x11,
	font_vga_8x16,
	font_vga_8x8;

extern const struct gfont *fonts[];

#define for_each_font(X)	for ((X) = fonts; *(X); (X)++)

#endif /* _GFB_FONT_H */
