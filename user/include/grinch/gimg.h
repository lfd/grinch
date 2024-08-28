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

#ifndef _GIMG_H
#define _GIMG_H

#include <grinch/fb.h>
#include <grinch/gpaint.h>

struct gimg {
	unsigned int width;
	unsigned int height;
	size_t sz;
	unsigned char data[];
};

int gimg_load(const char *fname, struct gimg **img);
void gimg_unload(struct gimg *img);
struct gcolor gimg_getpixel(struct gimg *img, struct gcoord c);

void gimg_to_fb(void *fb, struct grinch_fb_screeninfo *info, struct gimg *img,
		struct gcoord off);

#endif /* _GIMG_H */
