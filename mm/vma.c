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

#define dbg_fmt(x)	"kmm: " x

#include <asm_generic/grinch_layout.h>

#include <grinch/bitmap.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/kmm.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/types.h>
#include <grinch/vma.h>

static unsigned long *pmem_bitmap;
static paddr_t pmem_base;
static size_t pmem_pages;

static inline bool is_vma_pmem(paddr_t addr)
{
	if (addr >= pmem_base && addr < pmem_base + pmem_pages * PAGE_SIZE)
		return true;
	return false;
}

static unsigned int vma_pmem_mark_used(paddr_t addr, size_t pages)
{
	unsigned int start;

	if (addr & PAGE_OFFS_MASK)
		return -ERANGE;

	if (!is_vma_pmem(addr))
		return -EINVAL;

	if (addr + pages * PAGE_SIZE >= pmem_base + pmem_pages * PAGE_SIZE)
		return -ERANGE;

	start = (addr - pmem_base) / PAGE_SIZE;

	bitmap_set(pmem_bitmap, start, pages);

	return 0;
}

int vma_init(paddr_t addrp, size_t sizep)
{
	int err;

	pr("Found main memory: %llx, size: %lx\n", addrp, sizep);
	pmem_base = addrp;
	pmem_pages = PAGES(sizep);

	if (pmem_pages % 64 != 0) {
		pr("Implement unaligned bitmap\n");
		return -ERANGE;
	}

	pmem_bitmap = kmm_page_zalloc(PAGES(page_up(BITMAP_SIZE(pmem_pages))));
	if (!pmem_bitmap)
		return -ENOMEM;

	/* check if the physical location of the OS is inside that region */
	err = vma_pmem_mark_used(kmm_v2p((void*)VMGRINCH_BASE),
				 PAGES(GRINCH_SIZE));
	if (err && err != -ERANGE)
		return err;

	return err;
}

int vma_init_fdt(void)
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

	err = vma_init(addrp, sizep);
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
		vma_pmem_mark_used(addrp, PAGES(page_up(sizep)));
	}

	return err;
}
