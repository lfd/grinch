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

#define dbg_fmt(x)	"initrd: " x

#include <grinch/alloc.h>
#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/initrd.h>
#include <grinch/printk.h>
#include <grinch/utils.h>

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

struct initrd initrd;

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
	hdr->next_header = (void *)(((u64)hdr->next_header + 3) & ~3);

	if (hdr->body[-1] != 0)
		return -EINVAL;

	if (hdr->name_len == sizeof(CPIO_TRAILER) &&
	    strcmp(hdr->name, CPIO_TRAILER) == 0)
		return -ENOENT;

	return 0;
}

static int cpio_find_file(const char *pathname, struct cpio_header *hdr)
{
	const void *this;
	int err;

	this = initrd.vbase;
	if (!this)
		return -ENOENT;

	/* Skip leading slashes */
	while (*pathname == '/')
		pathname++;

	while (1) {
		err = parse_cpio_header(this, hdr);
		if (err)
			return err;
		this = hdr->next_header;
		if (!strcmp(hdr->name, pathname))
			return 0;
	}

	return -ENOENT;
}

static int __init cpio_size(const void *base, size_t *size)
{
	struct cpio_header hdr;
	const void *this;
	int err;

	this = base;
	while (1) {
		err = parse_cpio_header(this, &hdr);
		if (err)
			break;
		this = hdr.next_header;
	}

	if (this == base)
		return -ENOENT;

	*size = (((uintptr_t)hdr.body - (uintptr_t)base) + 255)
		& BIT_MASK(64, 8);

	return 0;
}

void *initrd_read_file(const char *pathname, size_t *len)
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
	if (len)
		*len = hdr.body_len;

	return ret;
}

int __init initrd_init(void)
{
	size_t initrd_pages;
	paddr_t page_start;
	int err, offset;
	u64 start, end;

	offset = fdt_path_offset(_fdt, ISTR("/chosen"));
	if (offset <= 0)
		return -ENOENT;

	err = fdt_read_u64(_fdt, offset, ISTR("linux,initrd-start"), &start);
	if (err)
		return -ENOENT;

	err = fdt_read_u64(_fdt, offset, ISTR("linux,initrd-end"), &end);
	if (err)
		return -ENOENT;

	initrd.pstart = start;
	initrd.vbase = p2v(initrd.pstart);

	initrd.size = end - start;
	pri("initrd: found at 0x%llx (SZ: 0x%lx)\n", initrd.pstart, initrd.size);
	err = cpio_size(initrd.vbase, &initrd.size);
	if (err)
		return err;
	pri("initrd: real size: 0x%lx\n", initrd.size);

	page_start = start & PAGE_MASK;
	initrd_pages = PAGES(page_up(start + initrd.size) - page_start);
	err = phys_mark_used(page_start, initrd_pages);
	if (err) {
		pri("Error reserving memory for ramdisk\n");
		return err;
	}

	return 0;
}
