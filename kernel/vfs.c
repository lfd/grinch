/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"vfs: " x

#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/vfs.h>

extern unsigned char __initrd_start[];
extern unsigned char __initrd_end[];

#define CPIO_MAGIC	"070701"
#define CPIO_TRAILER	"TRAILER!!!"
#define CPIO_ALIGN(len)	((((len) + 1) & ~3) + 2)

struct cpio_header {
	unsigned int name_len;
	unsigned int body_len;
	const char *name;
	const char *body;
	const void *next_header;
};

static int parse_cpio_header(const char *s, struct cpio_header *hdr)
{
	unsigned long parsed[12];
	char buf[9];
	int i;

	if (memcmp(s, CPIO_MAGIC, 6))
		return -EINVAL;

	s += 6;

	buf[8] = '\0';
	for (i = 0; i < 12; i++, s += 8) {
		memcpy(buf, s, 8);
		parsed[i] = strtoul(buf, NULL, 16);
	}

	s += 8;

	hdr->body_len = parsed[6];
	hdr->name_len = parsed[11];
	hdr->name = s;
	hdr->body = hdr->name + CPIO_ALIGN(hdr->name_len);
	hdr->next_header = hdr->body + hdr->body_len;

	if (hdr->body[-1] != 0)
		return -EINVAL;

	if (hdr->name_len == sizeof(CPIO_TRAILER) &&
	    strcmp(hdr->name, CPIO_TRAILER) == 0)
		return -ENOENT;

	return 0;
}

static int cpio_find_file(const char *pathname, struct cpio_header *hdr)
{
	const void *initrd;
	int err;

	initrd = __initrd_start;

	/* Skip leading slashes */
	while (*pathname == '/')
		pathname++;

	while (1) {
		err = parse_cpio_header(initrd, hdr);
		if (err)
			return err;
		initrd = hdr->next_header;
		if (!strcmp(hdr->name, pathname))
			return 0;
	}
	return -ENOENT;
}

static void *initrd_read_file(const char *pathname)
{
	struct cpio_header hdr;
	int err;
	void *ret;

	err = cpio_find_file(pathname, &hdr);
	if (err)
		return ERR_PTR(err);

	ret = kmalloc(hdr.body_len);
	if (!ret)
		return ERR_PTR(-ENOMEM);

	memcpy(ret, hdr.body, hdr.body_len);
	return ret;
}

void *vfs_read_file(const char *pathname)
{
	const char *ird = "initrd:";

	/* We only support the initrd file system at the moment */
	if (strncmp(pathname, ird, strlen(ird)))
		return ERR_PTR(-ENOSYS);

	return initrd_read_file(pathname + strlen(ird));
}
