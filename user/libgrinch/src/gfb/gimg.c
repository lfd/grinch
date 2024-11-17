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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grinch/minmax.h>
#include <grinch/gfb/gimg.h>

inline struct gcolor gimg_getpixel(struct gimg *img, struct gcoord c)
{
	struct gcolor ret;
	u8 *data;

	data = img->data + (24 / 8) * (c.y * img->width + c.x);

	ret.r = data[0];
	ret.g = data[1];
	ret.b = data[2];

	return ret;
}

void
gimg_to_fb(struct gfb_handle *h, struct gimg *img, struct gcoord off)
{
	struct gcoord c, max, dst;
	struct gcolor src;

	max.x = min(off.x + img->width, h->gfb->info.mode.xres) - off.x;
	max.y = min(off.y + img->height, h->gfb->info.mode.yres) - off.y;

	for (c.y = 0; c.y < max.y; c.y++)
		for (c.x = 0; c.x < max.x; c.x++) {
			src = gimg_getpixel(img, c);
			dst.y = c.y + off.y;
			dst.x = c.x + off.x;
			h->gfb->setpixel(h, dst, src);
		}
}

void gimg_unload(struct gimg *img)
{
	free(img);
}

int gimg_load(const char *fname, struct gimg **i)
{
	u32 width, height;
	struct gimg *img;
	ssize_t bread;
	int err, fd;
	size_t sz;

	fd = open(fname, O_RDONLY);
	if (fd == -1)
		return -errno;

	err = read(fd, &width, sizeof(width));
	err += read(fd, &height, sizeof(height));
	if (err != 8)
		goto close_out;

	sz = width * height * (24 / 8);
	img = malloc(sizeof(*img) + sz);
	if (!img) {
		err = -ENOMEM;
		goto close_out;
	}

	img->width = width;
	img->height = height;
	img->sz = sz;

	bread = read(fd, img->data, img->sz);
	if (bread == -1) {
		err = -errno;
		goto error_out;
	} else if ((size_t)bread != img->sz) {
		err = -EINVAL;
		goto error_out;
	}

	close(fd);
	*i = img;

	return 0;

error_out:
	gimg_unload(img);

close_out:
	close(fd);

	return err;
}
