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

#define dbg_fmt(x)	"mm: " x

#include <asm/grinch_layout.h>

#include <grinch/bitmap.h>
#include <grinch/errno.h>
#include <grinch/fdt.h>
#include <grinch/paging.h>
#include <grinch/mm.h>
#include <grinch/percpu.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/symbols.h>

#define BITMAP_BITS	PAGES(GRINCH_SIZE)

static unsigned long __internal_bitmap[BITMAP_SZ(BITMAP_BITS)];

#define MEMPOOL_INT	0
#define MEMPOOL_EXT	1

/*
 * At the moment, only two pools are supported. Those two pools are ID-mapped
 * and do NOT support non-contiguous regions. Hence, all v/p translations
 * inside the region can be translated with a simple offset, and no need for a
 * full PT walk.
 */
static struct mempool {
	paddr_t base;
	size_t pages;
	unsigned long *bitmap;

	ptrdiff_t v2p_offset;
	const void *vbase;
} mempool[2] = {
	/* first entry is the internal page pool */
	[MEMPOOL_INT] = {
		/* .base filled by mm_init() */
		.pages = BITMAP_BITS,
		.bitmap = __internal_bitmap,
		.vbase = _load_addr,
		/* .v2p_offset set in assembly */
	},
	[MEMPOOL_EXT] = {
		.vbase = (void*)KMEM_BASE,
		/* .v2p_offset set by mempool_init() */
	},
};

void mm_set_int_v2p_offset(ptrdiff_t off)
{
	mempool[MEMPOOL_INT].v2p_offset = off;
}

static __always_inline paddr_t
_virt_to_phys(struct mempool *m, const void *virt)
{
	return (paddr_t)virt - m->v2p_offset;
}

static __always_inline void * _phys_to_virt(struct mempool *m, paddr_t phys)
{
	return (void *)phys + m->v2p_offset;
}

static struct mempool *addr_to_mempool(const void *virt)
{
	struct mempool *m = NULL;

	if (virt >= _load_addr && virt < (_load_addr + GRINCH_SIZE))
		m = &mempool[MEMPOOL_INT];
	else if ((u64)virt >= KMEM_BASE && (u64)virt < (KMEM_BASE + KMEM_SIZE))
		m = &mempool[MEMPOOL_EXT];

	return m;
}

paddr_t virt_to_phys(const void *virt)
{
	struct mempool *m;
	const void *tpcpu = this_per_cpu();

	/*
	 * Check if we have a per_cpu address. If so, translate if to the
	 * corresponding MEMPOOL_INT address.
	 */
	if (virt >= tpcpu &&
	    virt < tpcpu + sizeof(struct per_cpu))
		virt = (void*)per_cpu(this_cpu_id()) + (virt - tpcpu);

	m = addr_to_mempool(virt);
	if (!m)
		return 0;

	return _virt_to_phys(m, virt);
}

void *phys_to_virt(paddr_t addr)
{
	struct mempool *m;
	unsigned int i;

	for (i = 0, m = &mempool[0]; i < ARRAY_SIZE(mempool); i++, m++) {
		if (addr >= m->base && addr < m->base + m->pages * PAGE_SIZE)
			return _phys_to_virt(m, addr);
	}

	return ERR_PTR(-ENOENT);
}

static int mempool_mark_free(struct mempool *m, paddr_t addr, size_t pages)
{
	unsigned int start;

	if (addr & PAGE_OFFS_MASK)
		return -EINVAL;

	if (addr < m->base || addr > (m->base + m->pages * PAGE_SIZE))
		return -ERANGE;

	start = (addr - m->base) / PAGE_SIZE;
	if (start + pages > m->pages)
		return -ERANGE;

	bitmap_clear(m->bitmap, start, pages);

	return 0;
}

static int mempool_mark_used(struct mempool *m, paddr_t addr, size_t pages)
{
	unsigned int start;

	if (addr & PAGE_OFFS_MASK)
		return -EINVAL;

	if (addr < m->base || addr > (m->base + m->pages * PAGE_SIZE))
		return -ERANGE;

	start = (addr - m->base) / PAGE_SIZE;
	if (start + pages > m->pages)
		return -ERANGE;

	bitmap_set(m->bitmap, start, pages);

	return 0;
}

static int mempool_init(struct mempool *m)
{
	unsigned long bitmap_size;
	unsigned int bitmap_pgs;

	if (m->base & MEGA_PAGE_OFFS_MASK)
		return trace_error(-ENOSYS);

	bitmap_size = BITMAP_SZ(m->pages);
	bitmap_pgs = PAGES(page_up(bitmap_size));

	m->v2p_offset = (paddr_t)m->vbase - m->base;

	m->bitmap = page_zalloc(bitmap_pgs, PAGE_SIZE, PAF_INT);
	if (IS_ERR(m->bitmap))
		return PTR_ERR(m->bitmap);

	return 0;
}

static int mempool_find_and_allocate(struct mempool *m, size_t pages,
				     unsigned int alignment, paddr_t *paddr)
{
	unsigned int start;

	switch (alignment) {
		case PAGE_SIZE:
			alignment = 0;
			break;
		case MEGA_PAGE_SIZE:
			alignment = PAGES(MEGA_PAGE_SIZE) - 1;
			break;
		case GIGA_PAGE_SIZE:
			alignment = PAGES(GIGA_PAGE_SIZE) - 1;
			break;
		default:
			return trace_error(-EINVAL);
	};

	start = bitmap_find_next_zero_area(m->bitmap, m->pages, 0, pages,
					   alignment);
	if (start > m->pages)
		return trace_error(-ENOMEM);

	/* mark as used, return pointer */
	bitmap_set(m->bitmap, start, pages);

	*paddr = m->base + start * PAGE_SIZE;
	return 0;
}

static int mempool_free(struct mempool *m, const void *addr, size_t pages)
{
	unsigned int start;

	if ((unsigned long)addr & PAGE_OFFS_MASK)
		return trace_error(-EINVAL);

	start = (addr - m->vbase) / PAGE_SIZE;
	bitmap_clear(m->bitmap, start, pages);

	return 0;
}

static inline struct mempool *get_mempool(paf_t paf)
{
	if (paf & PAF_INT)
		return &mempool[MEMPOOL_INT];
	else if (paf & PAF_EXT)
		return &mempool[MEMPOOL_EXT];

	return ERR_PTR(-ENOSYS);
}

int page_free(const void *addr, unsigned int pages, paf_t paf)
{
	struct mempool *m;

	m = get_mempool(paf);
	if (IS_ERR(m))
		return PTR_ERR(m);

	return mempool_free(m, addr, pages);
}

void *page_alloc(unsigned int pages, unsigned int alignment, paf_t paf)
{
	struct mempool *m;
	paddr_t paddr;
	void *ret;
	size_t sz;
	int err;

	m = get_mempool(paf);
	if (IS_ERR(m))
		return m;

	/* This allocator only supports simple contiguous virt/phys mappings */
	err = mempool_find_and_allocate(m, pages, alignment, &paddr);
	if (err)
		return ERR_PTR(err);

	sz = pages * PAGE_SIZE;
	ret = _phys_to_virt(m, paddr);
	if (paf & PAF_EXT) {
		/* In case of the ext mempool, we must create the mapping */
		err = map_range(this_root_table_page(), ret, paddr, sz,
				PAGE_FLAGS_MEM_RW);
		if (err)
			return ERR_PTR(err);
	}

	if (paf & PAF_ZERO)
		memset(ret, 0, sz);

	return ret;
}

int mm_init(void)
{
	struct mempool *m = &mempool[MEMPOOL_INT];
	int err;

	pr("OS pages: %lu\n", num_os_pages());
	pr("Internal page pool pages: %lu\n", internal_page_pool_pages());

	m->base = virt_to_phys(_load_addr);

	/* mark OS pages as used */
	err = mempool_mark_used(m, m->base, num_os_pages());
	if (err)
		return err;

	/* reserve pages for bootstrap page tables */
	err = mempool_mark_used(m, m->base + num_os_pages() * PAGE_SIZE,
				PIE_PAGES);

	return err;
}

int mm_init_late(paddr_t addrp, size_t sizep)
{
	struct mempool *m;
	int err;

	pr("Found main memory: %llx, size: %lx\n", addrp, sizep);
	m = &mempool[MEMPOOL_EXT];
	m->base = addrp;
	m->pages = PAGES(sizep);

	err = mempool_init(m);
	if (err)
		return err;

	/* check if the physical location of the OS is inside that region */
	err = mempool_mark_used(m, virt_to_phys(_load_addr),
				PAGES(GRINCH_SIZE));
	if (err && err != -ERANGE)
		return err;

	return err;
}

int mm_init_late_fdt(void)
{
	int child, err, len, memory, ac, sc, parent;
	struct mempool *m;
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

	m = &mempool[MEMPOOL_EXT];
	err = mm_init_late(addrp, sizep);
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
		err = mempool_mark_used(m, addrp, PAGES(page_up(sizep)));
		if (err)
			return err;
	}

	/* Free bootstrapping pages */
	m = &mempool[MEMPOOL_INT];
	err = mempool_mark_free(m, m->base + num_os_pages() * PAGE_SIZE,
				PIE_PAGES);

	return err;
}

int page_mark_used(void *addr, size_t pages)
{
	struct mempool *m;
	paddr_t paddr;

	m = addr_to_mempool(addr);
	if (!m)
		return -EINVAL;

	paddr = _virt_to_phys(m, addr);

	return mempool_mark_used(m, paddr, pages);
}
