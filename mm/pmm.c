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

#define dbg_fmt(x)	"pmm: " x

#include <asm_generic/grinch_layout.h>
#include <asm/spinlock.h>

#include <grinch/bitmap.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/kmm.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/types.h>
#include <grinch/mm.h>
#include <grinch/pmm.h>

static struct bitmap pmm_bitmap;
static paddr_t pmm_base, pmm_end;
static DEFINE_SPINLOCK(pmm_lock);

static inline bool is_pmm(paddr_t addr)
{
	if (addr >= pmm_base && addr < pmm_end)
		return true;
	return false;
}

static int pmm_check(paddr_t addr, size_t pages)
{
	if (addr & PAGE_OFFS_MASK)
		return -ERANGE;

	if (!is_pmm(addr))
		return -EINVAL;

	if (addr + pages * PAGE_SIZE >= pmm_end)
		return -ERANGE;

	return 0;
}

static int pmm_mark_used(paddr_t addr, size_t pages)
{
	unsigned int start;
	int err;

	err = pmm_check(addr, pages);
	if (err)
		return err;

	start = (addr - pmm_base) / PAGE_SIZE;
	bitmap_set(pmm_bitmap.bitmap, start, pages);

	return 0;
}

int pmm_page_free(paddr_t addr, size_t pages)
{
	unsigned int start;
	int err;

	err = pmm_check(addr, pages);
	if (err)
		return err;

	start = (addr - pmm_base) / PAGE_SIZE;
	bitmap_clear(pmm_bitmap.bitmap, start, pages);

	return 0;
}

int pmm_page_alloc_aligned(paddr_t *res, size_t pages, unsigned int alignment,
			   paddr_t hint)
{
	unsigned int page_offset;
	int err;

	if (hint)  {
		if (!is_pmm(hint) || hint & PAGE_OFFS_MASK)
			return -EINVAL;

		page_offset = (hint - pmm_base) / PAGE_SIZE;
	} else {
		page_offset = 0;
	}

	spin_lock(&pmm_lock);
	err = mm_bitmap_find_and_allocate(&pmm_bitmap, pages, page_offset, alignment);
	if (err < 0)
		return err;
	spin_unlock(&pmm_lock);

	*res = pmm_base + err * PAGE_SIZE;
	return 0;
}

int pmm_init(paddr_t addrp, size_t sizep)
{
	int err;

	pr("Found main memory: %llx, size: %lx\n", addrp, sizep);
	pmm_base = addrp;
	pmm_bitmap.bit_max = PAGES(sizep);

	if (pmm_bitmap.bit_max % 64 != 0) {
		pr("Implement unaligned bitmap\n");
		return -ERANGE;
	}

	pmm_bitmap.bitmap = kmm_page_zalloc(
		PAGES(page_up(BITMAP_SIZE(pmm_bitmap.bit_max))));
	if (!pmm_bitmap.bitmap)
		return -ENOMEM;
	pmm_end = pmm_base + pmm_bitmap.bit_max * PAGE_SIZE;

	/* check if the physical location of the OS is inside that region */
	err = pmm_mark_used(kmm_v2p((void*)VMGRINCH_BASE), PAGES(GRINCH_SIZE));
	if (err && err != -ERANGE)
		return err;

	return err;
}

int pmm_init_fdt(void)
{
	int child, err, len, memory, ac, sc, parent;
	const char *uname;
	const void *reg;
	paddr_t addrp;
	size_t sizep;

	memory = fdt_path_offset(_fdt, "/memory");
	if (memory <= 0) {
		pr("NO MEMORY NODE FOUND! TRYING TO CONTINUE\n");
		return 0;
	}

	reg = fdt_getprop(_fdt, memory, "reg", &len);
	if (IS_ERR(reg))
		trace_error(-EINVAL);

	parent = fdt_parent_offset(_fdt, memory);
	if (parent < 0)
		trace_error(-EINVAL);
	ac = fdt_address_cells(_fdt, parent);
	if (ac < 0)
		return ac;
	sc = fdt_size_cells(_fdt, parent);
	if (sc < 0)
		return sc;

	ac = (ac + sc) * sizeof(fdt32_t);

	if (len % ac != 0 && len / ac != 1) {
		pr("MULTIPLE CELLS NOT SUPPORTED\n");
		return trace_error(-EINVAL);
	}

	err = fdt_read_reg(_fdt, memory, 0, &addrp, (u64*)&sizep);
	if (err < 0)
		return trace_error(err);

	err = pmm_init(addrp, sizep);
	if (err)
		return trace_error(err);

	/* search for reserved regions in the device tree */
	memory = fdt_path_offset(_fdt, "/reserved-memory");
	if (memory <= 0) {
		pr("NO RESERVED MEMORY NODE FOUND! TRYING TO CONTINUE\n");
		return 0;
	}

	fdt_for_each_subnode(child, _fdt, memory) {
		if (!fdt_device_is_available(_fdt, child))
			continue;

		uname = fdt_get_name(_fdt, child, NULL);

		err = fdt_read_reg(_fdt, child, 0, &addrp, (u64*)&sizep);
		if (err) {
			pr("Error reserving area %s\n", uname);
			return trace_error(err);
		}

		pr("Reserving memory area %s (0x%llx len: 0x%lx)\n",
		   uname, addrp, sizep);
		pmm_mark_used(addrp, PAGES(page_up(sizep)));
	}

	return err;
}
