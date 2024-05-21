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
#include <grinch/fs/initrd.h>
#include <grinch/fs/util.h>
#include <grinch/gfp.h>
#include <grinch/minmax.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>
#include <grinch/utils.h>

#define CPIO_MAGIC	"070701"
#define CPIO_TRAILER	"TRAILER!!!"
#define CPIO_ALIGN(len)	((((len) + 1) & ~3) + 2)

struct cpio_header {
	unsigned int name_len;
	unsigned int body_len;
	unsigned short mode;
	const char *name;
	const char *body;
	const void *next_header;
};

struct cpio_context {
	struct cpio_header hdr;
	unsigned int subdir_level;
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

	hdr->mode = parsed[1];
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

	/* If we have the root directory, replace it with "." */
	if (strlen(pathname) == 0)
		pathname = ".";

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

static ssize_t initrd_read(struct file_handle *handle, char *buf, size_t count)
{
	struct cpio_context *ctx;
	struct cpio_header *hdr;
	unsigned long copied;
	const void *src;
	struct file *fp;
	loff_t *off;
	size_t sz;

	fp = handle->fp;
	off = &handle->position;
	ctx = fp->drvdata;
	hdr = &ctx->hdr;

	if (!S_ISREG(hdr->mode))
		return -EBADF;

	if (*off >= hdr->body_len)
		return 0;

	if (*off < 0)
		return -EINVAL;

	src = hdr->body + *off;
	sz = min((unsigned long)hdr->body_len - *off, count);
	if (handle->flags.is_kernel) {
		memcpy(buf, src, sz);
		copied = sz;
	} else {
		copied = copy_to_user(current_task(), buf, src, sz);
	}

	*off += copied;

	return copied;
}

static void initrd_close(struct file *fp)
{
	struct cpio_ctx *ctx;

	ctx = fp->drvdata;
	kfree(ctx);
}

static unsigned int get_subdir_level(struct cpio_header *hdr)
{
	if (!strcmp(hdr->name, "."))
		return 0;

	return 1 + strcount(hdr->name, '/');
}

static int
initrd_getdents(struct file_handle *handle, struct grinch_dirent *udents,
		unsigned int size)
{
	const char *entry_name, *prefix;
	struct cpio_header *hdr, tmp;
	struct grinch_dirent dent;
	struct cpio_context *ctx;
	unsigned int prefix_len;
	const void *this;
	struct file *fp;
	loff_t off, i;
	int err;

	fp = handle->fp;
	ctx = fp->drvdata;
	hdr = &ctx->hdr;
	if (!S_ISDIR(hdr->mode))
		return -ENOTDIR;

	off = handle->position;

	/* Skip n+1 positions */
	tmp = *hdr;
	off++;
	for (i = 0; i < off; i++) {
		this = tmp.next_header;
		err = parse_cpio_header(this, &tmp);
		if (err == -ENOENT) /* end of initrd */
			return 0;
		if (err)
			return err;
	}

	/* Skip potential subdirs */
	while (get_subdir_level(&tmp) != (ctx->subdir_level + 1)) {
		this = tmp.next_header;
		err = parse_cpio_header(this, &tmp);
		if (err == -ENOENT) /* end of initrd */
			return 0;
		if (err)
			return err;
		off++;
	}

	if (ctx->subdir_level == 0) {
		prefix = "";
		prefix_len = 0;
	} else {
		prefix = hdr->name;
		prefix_len = hdr->name_len - 1;
	}

	if (strncmp(prefix, tmp.name, prefix_len))
		return 0;

	entry_name = tmp.name + prefix_len;
	if (*entry_name == '/')
		entry_name++;

	switch (tmp.mode & S_IFMT) {
		case S_IFREG:
			dent.type = DT_REG;
			break;

		case S_IFDIR:
			dent.type = DT_DIR;
			break;

		default:
			dent.type = DT_UNKNOWN;
			break;
	}

	err = copy_dirent(udents, handle->flags.is_kernel, &dent, entry_name,
			  size);
	if (err)
		return err;

	handle->position = off;
	return 1;
}

static const struct file_operations initrd_fops = {
	.read = initrd_read,
	.close = initrd_close,
	.getdents = initrd_getdents,
};

static int initrd_open(const struct file_system *fs, struct file *filep,
		       const char *path, struct fs_flags flags)
{
	struct cpio_context *ctx;
	struct cpio_header *hdr;
	int err;

	if (flags.may_write)
		return -EPERM;

	filep->fops = &initrd_fops;
	filep->drvdata = kmalloc(sizeof(struct cpio_context));
	if (!filep->drvdata)
		return -ENOMEM;

	ctx = filep->drvdata;
	hdr = &ctx->hdr;
	err = cpio_find_file(path, hdr);
	if (err)
		goto err_out;

	if (flags.must_directory && !S_ISDIR(hdr->mode)) {
		err = -ENOTDIR;
		goto err_out;
	}

	ctx->subdir_level = get_subdir_level(hdr);

	return 0;

err_out:
	kfree(ctx);
	return err;
}

static int
initrd_stat(const struct file_system *fs, const char *pathname, struct stat *st)
{
	struct cpio_header hdr;
	int err;

	err = cpio_find_file(pathname, &hdr);
	if (err)
		return err;

	st->st_size = hdr.body_len;
	st->st_mode = hdr.mode;

	return 0;
}

static const struct file_system_operations fs_ops_initrd = {
	.open_file = initrd_open,
	.stat = initrd_stat,
};

const struct file_system initrdfs = {
	.fs_ops = &fs_ops_initrd,
};
