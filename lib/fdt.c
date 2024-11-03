/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x) "fdt: " x

#include <grinch/device.h>
#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/ioremap.h>
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

int fdt_match_device_off(const void *fdt, int offset,
			 const struct of_device_id *compats,
			 const struct of_device_id **match)
{
	const struct of_device_id *compat;
	const char *compatible;
	size_t len;
	int res;

	compatible = fdt_getprop(_fdt, offset, "compatible", &res);
	if (res < 0)
		return -EINVAL;

	do {
		compat = fdt_find_compat(compatible, compats);
		if (compat && fdt_device_is_available(_fdt, offset)) {
			if (match)
				*match = compat;
			return offset; /* we found it */
		}

		len = strlen(compatible) + 1;
		compatible += len;
		res -= len;
	} while (res);

	return -ENOENT;
}

int fdt_find_device(const void *fdt, const char *path,
		    const struct of_device_id *compats,
		    const struct of_device_id **match)
{
	int off, sub;

	if (!path)
		off = 0;
	else {
		off = fdt_path_offset(fdt, path);
		if (off <= 0)
			return -ENOENT;
	}

	fdt_for_each_subnode(sub, _fdt, off)
		if (fdt_match_device_off(fdt, sub, compats, match) > 0)
			return sub;

	return -ENOENT;
}

int __init fdt_init(paddr_t pfdt)
{
	void *fdt;
	int err;

	/* be pessimistic and remap 2 MiB */
	fdt = ioremap(pfdt, MEGA_PAGE_SIZE);
	if (IS_ERR(fdt))
		return PTR_ERR(fdt);

	err = fdt_check_header(fdt);
	if (err) {
		pri("No valid FDT header behind 0x%lx\n", pfdt);
		err = -EINVAL;
		goto unmap;
	}

	err = fdt_totalsize(fdt);
	if (err <= 0) {
		pri("FDT totalsize\n");
		err = -EINVAL;
		goto unmap;
	}
	pri("FDT size: %u\n", err);

	_fdt = alloc_pages(PAGES(page_up(err)));
	if (!_fdt) {
		err = -ENOMEM;
		goto unmap;
	}

	err = fdt_move(fdt, _fdt, err);
	if (err)
		pri("FDT move failed\n");

unmap:
	if (iounmap(fdt, MEGA_PAGE_SIZE))
		pri("iounmap failed\n");
	return err;
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

int fdt_addr_sz(const void *fdt, int off, int *_ac, int *_sc)
{
	int ac, sc;

	ac = fdt_address_cells(fdt, off);
	if (ac < 0)
		return ac;

	sc = fdt_size_cells(fdt, off);
	if (sc < 0)
		return sc;

	*_ac = ac;
	*_sc = sc;

	return 0;
}

int fdt_read_reg(const void *fdt, int nodeoffset, int idx,
		 struct mmio_area *area)
{
	int ac, sc, parent, res, reg_stride;
	const fdt32_t *reg;
	u64 addr, size;

	reg = fdt_getprop(fdt, nodeoffset, "reg", &res);
	if (res < 0)
		return res;

	parent = fdt_parent_offset(fdt, nodeoffset);
	if (parent == -FDT_ERR_NOTFOUND) /* an node without a parent does
					  * not have _any_ number of cells */
		return -FDT_ERR_BADNCELLS;
	if (parent < 0)
		return parent;

	res = fdt_addr_sz(fdt, parent, &ac, &sc);
	if (res < 0)
		return res;

	reg_stride = ac + sc;

	/* res is the number of bytes read and must be an even multiple of the
	 * sum of address cells and size cells */
	if ((res % (reg_stride * sizeof(*reg))) != 0)
		return -FDT_ERR_BADVALUE;

	res = _fdt_read_cells(&reg[reg_stride * idx], ac, &addr);
	if (res < 0)
		return res;

	res = _fdt_read_cells(&reg[ac + reg_stride * idx], sc, &size);
	if (res < 0)
		return res;

	area->paddr = addr;
	area->size = size;

	return 0;
}

int fdt_read_u64(const void *fdt, int nodeoffset, const char *name, u64 *res)
{
	const fdt64_t *prop;
	int len;

	prop = fdt_getprop(_fdt, nodeoffset, name, &len);
	if (!prop)
		return -ENOENT;

	switch (len) {
		case 4:
			*res = fdt32_to_cpu(*prop);
			break;

		case 8:
			*res = fdt64_to_cpu(*prop);
			break;

		default:
			return -EINVAL;
	}

	return 0;
}

int fdt_read_u32_array(const void *fdt, int nodeoffset, const char *name,
		       u32 *array, size_t min, size_t sz)
{
	const int *res;
	size_t count;
	int len;

        res = fdt_getprop(_fdt, nodeoffset, name, &len);
	if (!res)
		return -ENOENT;

	if ((len / sizeof(*array)) - min < sz)
		return -ERANGE;

	res += min;
	count = sz;
	while (count--) {
		*array = fdt32_to_cpu(*res);
		array++, res++;
	}

	return sz;
}

int fdt_irq_get(struct device *dev)
{
	const int *res;

	res = fdt_getprop(_fdt, dev->of.node, ISTR("interrupts"), NULL);
	if (IS_ERR(res))
		return PTR_ERR(res);

	return fdt32_to_cpu(*res);
}

int fdt_property_reg_u64_simple(void *fdt, const char *name, u64 addr, u64 sz)
{
	uint32_t reg[4];

	reg[0] = cpu_to_fdt32((addr >> 32) & 0xffffffff);
	reg[1] = cpu_to_fdt32(addr & 0xffffffff);
	reg[2] = cpu_to_fdt32((sz >> 32) & 0xffffffff);
	reg[3] = cpu_to_fdt32(sz & 0xffffffff);

	return fdt_property(fdt, name, reg, sizeof(reg));
}
