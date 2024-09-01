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

#include <string.h>

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

static void gfont_putc(struct gfb_handle *h, const struct gfont *desc,
		       struct gcolor color, struct gcoord coord, char c)
{
	struct gcoord dest;
	unsigned int y, x;
	const u16 *line;

	if (c >= desc->charcount) // actually - place empty character?
		return;

	line = desc->data + c * desc->height;
	for (y = 0; y < desc->height; y++) {
		for (x = 0; x < desc->width; x++) {
			if (*line & (1 << x)) {
				dest.y = coord.y;
				dest.x = coord.x + x;
				h->gfb->setpixel(h, dest, color);
			}
		}
		coord.y++;
		line++;
	}
}

/* returns the number of characters that were not printed */
int gfont_puts(struct gfb_handle *h, const struct gfont *desc,
	       struct gcolor color, struct gcoord coord, const char *s)
{
	unsigned int i, max;

	if (coord.y + desc->height > h->gfb->info.mode.yres)
		return strlen(s);

	max = (h->gfb->info.mode.xres - coord.x) / desc->width;
	for (i = 0; s[i] && i < max; i++) {
		gfont_putc(h, desc, color, coord, s[i]);
		coord.x += desc->width;
	}

	return strlen(s + i);
}
