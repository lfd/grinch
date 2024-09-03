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

#include <grinch/gfb/gfb.h>

#define DEF_SP(name) 								\
	static void setpixel_##name(struct gfb_handle *h, struct gcoord coord,	\
				    struct gcolor color)

static inline void *
px_offset(struct gfb_handle *h, struct gcoord *coord, unsigned int bpp)
{
	return h->fb + (bpp / 8) * (coord->y * h->gfb->info.mode.xres + coord->x);
}

DEF_SP(xrgb)
{
	u32 *dst;

	dst = px_offset(h, &coord, 32);

	*dst = (color.r << 16) |
	       (color.g << 8) |
	       (color.b << 0);
}

DEF_SP(rgb)
{
	u8 *dst;

	dst = px_offset(h, &coord, 24);

	dst[0] = color.b;
	dst[1] = color.g;
	dst[2] = color.r;
}

DEF_SP(rbg)
{
	u8 *dst;

	dst = px_offset(h, &coord, 24);

	dst[0] = color.g;
	dst[1] = color.b;
	dst[2] = color.r;
}

DEF_SP(r5g6b5)
{
	u16 *dst;

	dst = px_offset(h, &coord, 16);

	*dst = ((color.r >> 3) << 11) |
	       ((color.g >> 2) << 5) |
	       ((color.b >> 3) << 0);
}

DEF_SP(r5g5b5)
{
	u16 *dst;

	dst = px_offset(h, &coord, 16);

	*dst = ((color.r >> 3) << 10) |
	       ((color.g >> 3) << 5) |
	       ((color.b >> 3) << 0);
}

static const char *pixmode_string(pixmode_t mode)
{
	switch (mode) {
		case GFB_PIXMODE_XRGB:
			return "XRGB";

		case GFB_PIXMODE_RGB:
			return "RGB";

		case GFB_PIXMODE_RBG:
			return "RBG";

		case GFB_PIXMODE_R5G6B5:
			return "R5G6B5";

		case GFB_PIXMODE_R5G5B5:
			return "R5G5B5";

		default:
			return "Unknown mode";
	}
}

static int gfb_screeninfo(struct gfb *fb)
{
	int err;

	err = ioctl(fb->fd, GFB_IOCTL_SCREENINFO, &fb->info);
	if (err)
		return err;

	switch (fb->info.mode.pixmode) {
		case GFB_PIXMODE_XRGB:
			fb->setpixel = setpixel_xrgb;
			break;

		case GFB_PIXMODE_RGB:
			fb->setpixel = setpixel_rgb;
			break;

		case GFB_PIXMODE_RBG:
			fb->setpixel = setpixel_rbg;
			break;

		case GFB_PIXMODE_R5G6B5:
			fb->setpixel = setpixel_r5g6b5;
			break;

		case GFB_PIXMODE_R5G5B5:
			fb->setpixel = setpixel_r5g5b5;
			break;

		default:
			/*
			 * can not happen, but this will at least cause a null
			 * pointer exception somewhere else.
			 */
			fb->setpixel = NULL;
			break;
	}

	return 0;
}

int gfb_modeset(struct gfb *fb, struct gfb_mode *mode)
{
	int err;

	err = ioctl(fb->fd, GFB_IOCTL_MODESET, mode);
	if (err == -1)
		return -errno;

	err = gfb_screeninfo(fb);
	if (err == -1)
		return -errno;

	return 0;
}

void gfb_modeinfo(struct gfb *fb)
{
	printf("Screen resolution: %ux%u, Bits per Pixel: %u, Pixelmode: %s"
	       " framebuffer size: 0x%x\n",
	       fb->info.mode.xres, fb->info.mode.yres, fb->info.bpp,
	       pixmode_string(fb->info.mode.pixmode), fb->info.fb_size);
}

int gfb_open(struct gfb *fb, const char *dev)
{
	int err;

	fb->fd = open(dev, O_WRONLY);
	if (fb->fd == -1)
		return -errno;

	err = gfb_screeninfo(fb);
	if (err)
		return -errno;

	return 0;
}

void gfb_close(struct gfb *fb)
{
	close(fb->fd);
}
