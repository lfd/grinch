/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "fdt: " x

#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/ioremap.h>
#include <grinch/kmm.h>
#include <grinch/paging.h>
#include <grinch/printk.h>

unsigned char *_fdt;

bool fdt_device_is_available(const void *fdt, unsigned long node)
{
	const char *status = fdt_getprop(fdt, node, "status", NULL);

	if (!status)
		return true;

	if (!strcmp(status, "ok") || !strcmp(status, "okay"))
		return true;

	return false;
}

static const struct of_device_id
*fdt_find_compat(const char *str, const struct of_device_id *compats)
{
	for (; *compats->compatible; compats++)
		if (!strcmp(str, compats->compatible))
			return compats;
	return NULL;
}

int fdt_find_device(const void *fdt, const char *path,
		    const struct of_device_id *compats,
		    const struct of_device_id **match)
{
	const struct of_device_id *compat;
	const char *compatible;
	int off, sub, res;

	off = fdt_path_offset(_fdt, path);
	if (off <= 0)
		return -ENOENT;

	fdt_for_each_subnode(sub, _fdt, off) {
		compatible = fdt_getprop(_fdt, sub, "compatible", &res);
		if (res < 0)
			continue;

		compat = fdt_find_compat(compatible, compats);
		if (compat && fdt_device_is_available(_fdt, sub)) {
			if (match)
				*match = compat;
			return sub; /* we found it */
		}

	}

	return -ENOENT;
}

int fdt_init(paddr_t pfdt)
{
	void *fdt;
	int err;

	/* be pessimistic and remap 2 MiB */
	fdt = ioremap(pfdt, MEGA_PAGE_SIZE);
	if (IS_ERR(fdt))
		return PTR_ERR(fdt);

	err = fdt_totalsize(fdt);
	if (err <= 0) {
		ps("FDT totalsize\n");
		goto unmap;
	}

	_fdt = kmm_page_alloc(PAGES(page_up(err)));
	if (IS_ERR(_fdt))
		return PTR_ERR(_fdt);

	pr("DTB size: %u\n", err);

	err = fdt_move(fdt, _fdt, err);
	if (err)
		ps("FDT move failed\n");

unmap:
	return iounmap(fdt, MEGA_PAGE_SIZE);
}

static int _fdt_read_cells(const fdt32_t *cells, unsigned int n, uint64_t *value)
{
	unsigned int i;

	if (n > 2)
		return -FDT_ERR_BADNCELLS;

	*value = 0;
	for (i = 0; i < n; i++) {
		*value <<= (sizeof(*cells) * 8);
		*value |= fdt32_to_cpu(cells[i]);
	}

	return 0;
}

int fdt_read_reg(const void *fdt, int nodeoffset, int idx,
		 void *addrp, u64 *sizep)
{
	int parent;
	int ac, sc, reg_stride;
	int res;
	const fdt32_t *reg;

	reg = fdt_getprop(fdt, nodeoffset, "reg", &res);
	if (res < 0)
		return res;

	parent = fdt_parent_offset(fdt, nodeoffset);
	if (parent == -FDT_ERR_NOTFOUND) /* an node without a parent does
					  * not have _any_ number of cells */
		return -FDT_ERR_BADNCELLS;
	if (parent < 0)
		return parent;

	ac = fdt_address_cells(fdt, parent);
	if (ac < 0)
		return ac;

	sc = fdt_size_cells(fdt, parent);
	if (sc < 0)
		return sc;

	reg_stride = ac + sc;

	/* res is the number of bytes read and must be an even multiple of the
	 * sum of address cells and size cells */
	if ((res % (reg_stride * sizeof(*reg))) != 0)
		return -FDT_ERR_BADVALUE;

	if (addrp) {
		res = _fdt_read_cells(&reg[reg_stride * idx], ac, addrp);
		if (res < 0)
			return res;
	}
	if (sizep) {
		res = _fdt_read_cells(&reg[ac + reg_stride * idx], sc, sizep);
		if (res < 0)
			return res;
	}

	return 0;
}
