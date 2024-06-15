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

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <grinch/fb.h>

#include "fb.h"

#define XRES	320
#define YRES	240
#define BPP	32

static u32 buf[XRES * YRES];

void show_logo(void)
{
	struct grinch_fb_modeinfo mode = {
		.xres = XRES,
		.yres = YRES,
		.bpp = BPP,
	};
	struct grinch_fb fb;
	int err, fd;

	fd = open("/initrd/logo.raw", O_RDONLY);
	if (fd == -1)
		return;

	read(fd, buf, sizeof(buf));
	close(fd);

	err = grinch_fb_open(&fb, "/dev/fb0");
	if (err)
		return;

	err = grinch_fb_modeset(&fb, &mode);
	if (err)
		return;

	printf("Screen resolution %ux%u\n",
	       fb.info.mode.xres, fb.info.mode.yres);

	write(fb.fd, buf, sizeof(buf));

	grinch_fb_close(&fb);
}
