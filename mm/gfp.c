/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"gfp: " x

#include <asm/spinlock.h>

#include <grinch/bitmap.h>
#include <grinch/fdt.h>
#include <grinch/gfp.h>
#include <grinch/mm.h>
#include <grinch/paging.h>
#include <grinch/percpu.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/symbols.h>
#include <grinch/uaccess.h>

/*
 * Pages the kernel sets aside for allocations made before the memory of the
 * machine is known: early page tables, and the bitmap that will describe
 * that memory once found.
 */
static unsigned char early_pool[CONFIG_EARLY_POOL_PAGES][PAGE_SIZE]
	__aligned(PAGE_SIZE);

static DEFINE_SPINLOCK(gfp_lock);

struct memory_area {
	struct bitmap bitmap;
	struct {
		paddr_t base;
		paddr_t end;
	} p;
	struct {
		void *base;
		void *end;
	} v;
	bool valid;
};

/*
 * For the moment, we have two areas:
 *   0: Kernel Memory area. Here lives grinch, marked as used down to the
 *      early pool it carries, which serves the few allocations that must
 *      happen before the memory of the machine is known.
 *   1: Physical Memory. Here lives the whole physical memory. The whole
 *      grinch/KMM area is marked as used here.
 */
static struct memory_area memory_areas[2] =
{
	[0] = {
		.bitmap = {
			.bitmap = (unsigned long[BITS_TO_LONGS(KMM_PAGES_MAX)]){},
			// bit_max filled during initialisation
		},
		.p = {}, // filled during initialisation
		.v = {
			.base = __start,
			.end = __percpu_end,
		},
		/* Not before its physical range is known. */
		.valid = false,
	},
	[1] = {
	},
};
#define KMM_AREA	(&memory_areas[0])

#define for_each_valid_memory_area(IT, AREA)			\
	for((IT) = 0, (AREA) = memory_areas;			\
	    i < ARRAY_SIZE(memory_areas) && (AREA)->valid;	\
	    (IT)++, (AREA)++)

#define for_each_memory_area(IT, AREA)		\
	for((IT) = 0, (AREA) = memory_areas;	\
	    i < ARRAY_SIZE(memory_areas);	\
	    (IT)++, (AREA)++)

/* Backwards, so that the kernel's own area only serves last. */
#define for_each_valid_memory_area_reverse(IT, AREA)		\
	for ((IT) = ARRAY_SIZE(memory_areas); (IT)-- > 0;)	\
		if (!((AREA) = &memory_areas[(IT)])->valid)	\
			continue;				\
		else

/* only used once during initialisation */
void kmm_set_base(paddr_t pbase);

void kmm_set_base(paddr_t pbase)
{
	KMM_AREA->p.base = pbase;
}

size_t memory_size(void)
{
	const struct memory_area *a;

	a = &memory_areas[1];

	return a->p.end - a->p.base;
}

static inline paddr_t
memory_area_v2p(struct memory_area *area, const void *virt)
{
	return (paddr_t)(virt - area->v.base) + area->p.base;
}

static inline bool in_area(struct memory_area *area, const void *virt)
{
	return virt >= area->v.base && virt < area->v.end;
}

static inline bool
p_in_area(struct memory_area *area, paddr_t addr, unsigned int pages)
{
	if (addr < area->p.base)
		return false;

	if (!pages) {
		if (addr >= area->p.end)
			return false;
		return true;
	}

	if ((addr + PAGE_SIZE * pages) <= area->p.end)
		return true;

	return false;
}

static struct memory_area *find_area(const void *virt)
{
	struct memory_area *area;
	unsigned int i;

	for_each_valid_memory_area(i, area)
		if (in_area(area, virt))
			return area;
	return NULL;
}

static int memory_area_alloc_aligned(struct memory_area *area, ptrdiff_t *off,
				     unsigned int pages, unsigned int alignment,
				     paddr_t hint)
{
	long page_offset;
	int err;

	if (hint == INVALID_PHYS_ADDR && !off)
		panic("Invalid usage\n");

	/* hint MUST arrive sanity checked */
	if (hint != INVALID_PHYS_ADDR)
		page_offset = (hint - area->p.base) / PAGE_SIZE;
	else
		page_offset = -1;

	spin_lock(&gfp_lock);
	err = mm_bitmap_find_and_allocate(&area->bitmap, pages, page_offset,
					  alignment);
	spin_unlock(&gfp_lock);
	if (err < 0)
		return err;

	if (off)
		*off = err * PAGE_SIZE;

	return 0;
}

static int _alloc_pages_aligned(void **res, unsigned int pages,
			   unsigned int alignment, void *hint)
{
	struct memory_area *area;
	unsigned int i;
	paddr_t phint;
	ptrdiff_t off;
	int err;

	err = -EINVAL;
	off = 0;
	for_each_valid_memory_area_reverse(i, area) {
		if (!area->v.base)
			continue;

		if (hint) {
			if (!in_area(area, hint))
				continue;
			phint = memory_area_v2p(area, hint);
		} else
			phint = INVALID_PHYS_ADDR;

		// Ugly...
		err = memory_area_alloc_aligned(area, &off, pages, alignment,
						phint);
		if (!err)
			break;
	}

	if (!err && res)
		*res = area->v.base + off;
	return err;
}

int free_pages(const void *addr, unsigned int pages)
{
	struct memory_area *area;
	unsigned int start;

	if (page_voffset(addr))
		return -EINVAL;

	area = find_area(addr);
	if (!area)
		return -ENOENT;

	start = (addr - area->v.base) / PAGE_SIZE;
	if (start + pages > area->bitmap.bit_max)
		return -ERANGE;

	spin_lock(&gfp_lock);
	bitmap_clear(area->bitmap.bitmap, start, pages);
	spin_unlock(&gfp_lock);

	return 0;
}

static int
_phys_pages_alloc(paddr_t *res, size_t pages, unsigned int alignment,
		  paddr_t hint)
{
	struct memory_area *area;
	unsigned int i;
	ptrdiff_t off;
	int err;

	err = -EINVAL;
	for_each_valid_memory_area_reverse(i, area) {
		if (hint != INVALID_PHYS_ADDR) {
			if (p_in_area(area, hint, pages)) {
				err = memory_area_alloc_aligned(area, &off, pages, alignment, hint);
				if (err)
					return err;
				break;
			}
			continue;
		}

		// A simple pages-free check could help here
		err = memory_area_alloc_aligned(area, &off, pages, alignment,
						INVALID_PHYS_ADDR);
		if (!err)
			break;
	}

	if (!err && res)
		*res = area->p.base + off;
	return err;
}

int phys_free_pages(paddr_t phys, unsigned int pages)
{
	struct memory_area *area;
	unsigned int i, start;

	if (page_offset(phys))
		return -EINVAL;

	for_each_valid_memory_area(i, area) {
		if (p_in_area(area, phys, pages)) {
			start = (phys - area->p.base) / PAGE_SIZE;
			spin_lock(&gfp_lock);
			bitmap_clear(area->bitmap.bitmap, start, pages);
			spin_unlock(&gfp_lock);
			return 0;
		}
	}

	return -ERANGE;
}

int phys_pages_alloc(paddr_t *res, size_t pages, unsigned int alignment)
{
	return _phys_pages_alloc(res, pages, alignment, INVALID_PHYS_ADDR);
}

int phys_mark_used(paddr_t addr, size_t pages)
{
	return _phys_pages_alloc(NULL, pages, PAGE_SIZE, addr);
}

void *alloc_pages_aligned(unsigned int pages, unsigned int alignment)
{
	void *ret;
	int err;

	err = _alloc_pages_aligned(&ret, pages, alignment, 0);
	if (!err)
		return ret;
	return NULL;
}

paddr_t v2p(const void *virt)
{
	struct memory_area *area;
#ifdef CONFIG_MMU
	paddr_t phys;
#endif

	/*
	 * If virt is inside the grinch area of direct physical area, we can go
	 * through regular memora areas.
	 */
	area = find_area(virt);
	if (area)
		return memory_area_v2p(area, virt);

	if (is_uaddr(virt))
		panic("Don't resolve userspace addresses via v2p()!\n");

#ifdef CONFIG_MMU
	/*
	 * Perform a PTW walk. This could indeed be implemented more efficient.
	 * But we're usually not in a hot path here, so keep it simple.
	 */
	phys = paging_get_phys(kernel_root, virt);
	if (phys != INVALID_PHYS_ADDR)
		return phys;
#endif

	panic("Unable to resolve address %p\n", virt);
}

void *p2v(paddr_t phys)
{
	struct memory_area *area;
	unsigned int i;

	for_each_valid_memory_area(i, area)
		if (p_in_area(area, phys, 0) && area->v.base)
			return area->v.base + (phys - area->p.base);

	panic("Invalid phys address!\n");
}

int __init kernel_mem_init(void)
{
	struct memory_area *kmm = KMM_AREA;
	unsigned long pages, pool_start;

	pages = kernel_pages();
	kmm->bitmap.bit_max = pages;
	kmm->p.end = kmm->p.base + pages * PAGE_SIZE;

	pri("Kernel pages: %lu\n", pages);
	pri("Early pool pages: %u\n", CONFIG_EARLY_POOL_PAGES);

	/*
	 * Everything the kernel is stays taken; only the early pool serves.
	 * Slots of CPUs that never come are freed in arch_platform_init().
	 */
	bitmap_set(kmm->bitmap.bitmap, 0, pages);
	pool_start = PAGES((uintptr_t)early_pool - (uintptr_t)__start);
	bitmap_clear(kmm->bitmap.bitmap, pool_start, CONFIG_EARLY_POOL_PAGES);

	kmm->valid = true;

	return 0;
}

static int __init create_memory_area(paddr_t addrp, size_t sizep, void *virt)
{
	struct memory_area *area;
	paddr_t pgrinch;
	unsigned int i;
	int err;

	spin_lock(&gfp_lock);
	for_each_memory_area(i, area) {
		if (!area->valid)
			goto found_free_area;
	}
	spin_unlock(&gfp_lock);
	return -ENOENT;

found_free_area:
	spin_unlock(&gfp_lock);

	area->p.base = addrp;
	area->bitmap.bit_max = PAGES(sizep);
	area->p.end= addrp + area->bitmap.bit_max * PAGE_SIZE;

	area->bitmap.bitmap = zalloc_pages(
		PAGES(page_up(bitmap_size(area->bitmap.bit_max))));
	if (!area->bitmap.bitmap)
		return -ENOMEM;
	if (virt) {
		area->v.base = virt;
		area->v.end = area->v.base + area->bitmap.bit_max * PAGE_SIZE;
	} else
		area->v.base = area->v.end = NULL;

	/* check if the physical location of grinch is inside that region */
	pgrinch = KMM_AREA->p.base;
	if (pgrinch >= addrp && KMM_AREA->p.end < addrp + sizep) {
		err = memory_area_alloc_aligned(area, NULL,
						KMM_AREA->bitmap.bit_max,
						PAGE_SIZE, pgrinch);
		if (err)
			return err;
	}

	area->valid = true;

	return 0;
}

static int __init phys_mem_init(struct mmio_area *area)
{
	void *virt;
	int err;

	pri("Found main memory: %lx, size: %lx\n", area->paddr, area->size);
#ifdef CONFIG_MMU
	/*
	 * Create a direct physical R/W mapping, so that the kernel may easily
	 * access every single byte of physical memory.
	 */
	virt = (void *)DIR_PHYS_BASE;
	err = map_range(kernel_root, virt, area->paddr, area->size,
			GRINCH_MEM_RW);
	if (err)
		return err;
#else
	/* Untranslated, the direct mapping is the memory itself. */
	virt = (void *)(uintptr_t)area->paddr;
#endif

	err = create_memory_area(area->paddr, area->size, virt);

	return err;
}

int __init phys_mem_init_fdt(void)
{
	int child, err, len, memory, ac, sc, parent;
	struct mmio_area mem;
	paddr_t fstart, fend;
	const char *uname;
	const void *reg;

	memory = fdt_path_offset(_fdt, ISTR("/memory"));
	if (memory <= 0) {
		pri("NO MEMORY NODE FOUND! TRYING TO CONTINUE\n");
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
		pri("MULTIPLE CELLS NOT SUPPORTED\n");
		return trace_error(-EINVAL);
	}

	err = fdt_read_reg(_fdt, memory, 0, &mem);
	if (err)
		return trace_error(err);

	err = phys_mem_init(&mem);
	if (err)
		return trace_error(err);

	/*
	 * The device tree is read in place for as long as the system runs.
	 * The kernel's own window is already reserved as a whole, so only
	 * what reaches beyond it needs to be held.
	 */
	fstart = fdt_location & PAGE_MASK;
	fend = page_up(fdt_location + fdt_size());
	if (fstart >= KMM_AREA->p.base && fstart < KMM_AREA->p.end)
		fstart = KMM_AREA->p.end;
	if (fstart < fend) {
		err = phys_mark_used(fstart, PAGES(fend - fstart));
		if (err)
			return trace_error(err);
	}

	/* search for reserved regions in the device tree */
	memory = fdt_path_offset(_fdt, ISTR("/reserved-memory"));
	if (memory <= 0)
		return 0;

	fdt_for_each_subnode(child, _fdt, memory) {
		if (!fdt_device_is_available(_fdt, child))
			continue;

		uname = fdt_get_name(_fdt, child, NULL);

		err = fdt_read_reg(_fdt, child, 0, &mem);
		if (err) {
			pri("Error reserving area %s: %pe\n",
			    uname, ERR_PTR(err));
			continue;
		}

		pri("Reserving memory area %s (0x%lx len: 0x%lx)\n",
		   uname, mem.paddr, mem.size);
		phys_mark_used(mem.paddr, PAGES(page_up(mem.size)));
	}

	return 0;
}
