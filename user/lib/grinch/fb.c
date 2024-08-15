/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <grinch/fb.h>

static const char *pixmode_string(pixmode_t mode)
{
	switch (mode) {
		case GRINCH_FB_PIXMODE_XRGB:
			return "XRGB";

		case GRINCH_FB_PIXMODE_RGB:
			return "RGB";

		case GRINCH_FB_PIXMODE_R5G6B5:
			return "R5G6B5";

		case GRINCH_FB_PIXMODE_R5G5B5:
			return "R5G5B5";

		default:
			return "Unknown mode";
	}
}

static int grinch_fb_screeninfo(int fd, struct grinch_fb_screeninfo *info)
{
	return ioctl(fd, GRINCH_FB_SCREENINFO, info);
}

int grinch_fb_modeset(struct grinch_fb *fb, struct grinch_fb_modeinfo *mode)
{
	int err;

	err = ioctl(fb->fd, GRINCH_FB_MODESET, mode);
	if (err == -1)
		return -errno;

	err = grinch_fb_screeninfo(fb->fd, &fb->info);
	if (err == -1)
		return -errno;

	return 0;
}

void grinch_fb_modeinfo(struct grinch_fb *fb)
{
	printf("Screen resolution: %ux%u, Bits per Pixel: %u, Pixelmode: %s"
	       " framebuffer size: 0x%x\n",
	       fb->info.mode.xres, fb->info.mode.yres, fb->info.bpp,
	       pixmode_string(fb->info.mode.pixmode), fb->info.fb_size);
}

int grinch_fb_open(struct grinch_fb *fb, const char *dev)
{
	int err;

	fb->fd = open(dev, O_WRONLY);
	if (fb->fd == -1)
		return -errno;

	err = grinch_fb_screeninfo(fb->fd, &fb->info);
	if (err)
		return -errno;

	return 0;
}

void grinch_fb_close(struct grinch_fb *fb)
{
	close(fb->fd);
}
