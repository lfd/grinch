#ifndef _MM_H
#define _MM_H

#include <grinch/compiler_attributes.h>
#include <grinch/types.h>

typedef unsigned int paf_t;

int mm_init(void);

/*
 * After the internal page pool was set up, we're now able to access the DTB,
 * and setup the rest of the system's memory
 */
int mm_init_late(void);

/* page allocation flags */
#define PAF_INT		0x00000001
#define PAF_EXT		0x00000002
#define PAF_ZERO	0x80000000

void *page_alloc(unsigned int pages, unsigned int alignment, paf_t paf);
int page_free(const void *addr, unsigned int pages, paf_t paf);

static __always_inline
void *page_zalloc(unsigned int pages, unsigned int alignment, paf_t paf)
{
	return page_alloc(pages, alignment, paf  | PAF_ZERO);
}

/* only used once during initialisation */
void mm_set_int_v2p_offset(ptrdiff_t diff);


/* translators */
paddr_t virt_to_phys(const void *virt);
void *phys_to_virt(paddr_t phys);

/* helpers */
int page_mark_used(void *addr, size_t pages);

#endif
