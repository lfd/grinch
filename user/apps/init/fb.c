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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grinch/gfb/gimg.h>

#include "fb.h"

#define XRES	800
#define YRES	600

void show_logo(void)
{
	struct gfb_mode mode = {
		.xres = XRES,
		.yres = YRES,
	};
	struct gfb_handle h;
	struct gimg *logo;
	struct gcoord off;
	struct gfb gfb;
	int err;

	err = gfb_open(&gfb, "/dev/fb0");
	if (err)
		return;

	if (gfb_pixmode_supported(&gfb, GFB_PIXMODE_XRGB))
		mode.pixmode = GFB_PIXMODE_XRGB;
	else if (gfb_pixmode_supported(&gfb, GFB_PIXMODE_RGB))
		mode.pixmode = GFB_PIXMODE_RGB;
	else if (gfb_pixmode_supported(&gfb, GFB_PIXMODE_RBG))
		mode.pixmode = GFB_PIXMODE_RBG;

	err = gfb_modeset(&gfb, &mode);
	if (err)
		goto close_out;

	gfb_modeinfo(&gfb);

	err = gimg_load("/initrd/logo.gimg", &logo);
	if (err)
		goto close_out;

	h.gfb = &gfb;
	h.fb = malloc(gfb.info.fb_size);
	if (!h.fb)
		goto unload_out;

	off.x = (mode.xres - logo->width) / 2;
	off.y = (mode.yres - logo->height) / 2;
	gimg_to_fb(&h, logo, off);

	write(gfb.fd, h.fb, gfb.info.fb_size);

	free(h.fb);

unload_out:
	gimg_unload(logo);

close_out:
	gfb_close(&gfb);
}
