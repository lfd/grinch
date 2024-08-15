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

#include <grinch/gimg.h>

#include "fb.h"

#define XRES	800
#define YRES	600

void show_logo(void)
{
	struct grinch_fb_modeinfo mode = {
		.xres = XRES,
		.yres = YRES,
	};
	struct grinch_fb fb;
	struct gimg *logo;
	struct gcoord off;
	void *framebuffer;
	int err;

	err = grinch_fb_open(&fb, "/dev/fb0");
	if (err)
		return;

	if (grinch_fb_pixmode_supported(&fb, GRINCH_FB_PIXMODE_XRGB))
		mode.pixmode = GRINCH_FB_PIXMODE_XRGB;
	else if (grinch_fb_pixmode_supported(&fb, GRINCH_FB_PIXMODE_RGB))
		mode.pixmode = GRINCH_FB_PIXMODE_RGB;

	err = grinch_fb_modeset(&fb, &mode);
	if (err)
		goto close_out;

	grinch_fb_modeinfo(&fb);

	err = gimg_load("/initrd/logo.gimg", &logo);
	if (err)
		goto close_out;

	framebuffer = malloc(fb.info.fb_size);
	if (!framebuffer)
		goto unload_out;

	off.x = (mode.xres - logo->width) / 2;
	off.y = (mode.yres - logo->height) / 2;
	gimg_to_fb(framebuffer, &fb.info, logo, off);

	write(fb.fd, framebuffer, fb.info.fb_size);

	free(framebuffer);

unload_out:
	gimg_unload(logo);

close_out:
	grinch_fb_close(&fb);
}
