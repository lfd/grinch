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

#ifndef _GRINCH_FB_H
#define _GRINCH_FB_H

#include <grinch/fb_abi.h>

struct grinch_fb {
	int fd;
	struct grinch_fb_screeninfo info;
};

int grinch_fb_open(struct grinch_fb *fb, const char *dev);
void grinch_fb_close(struct grinch_fb *fb);

static inline bool
grinch_fb_pixmode_supported(struct grinch_fb *fb, pixmode_t mode)
{
	return fb->info.pixmodes_supported & (1UL << mode);
}

void grinch_fb_modeinfo(struct grinch_fb *fb);
int grinch_fb_modeset(struct grinch_fb *fb, struct grinch_fb_modeinfo *mode);

#endif /* _GRINCH_FB_H */
