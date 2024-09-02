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

#include <grinch/gfb/font.h>

const struct gfont *fonts[] = {
	&font_10x18,
	&font_6x10,
	&font_vga_6x11,
	&font_6x8,
	&font_7x14,
	&font_vga_8x16,
	&font_vga_8x8,
	&font_acorn_8x8,
	&font_mini_4x6,
	&font_pearl_8x8,
	&font_sun_12x22,
	&font_sun_8x16,
	&font_ter_16x32,
	NULL
};
