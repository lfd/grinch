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

#ifndef _PAGING_H
#define _PAGING_H

#include <grinch/const.h>
#include <grinch/types.h>
#include <grinch/utils.h>

#define PMASK(X)	(~((X) - 1))

/* Applies to both, arm64 and riscv64 */
#define PAGE_SHIFT		12
#define VPN_SHIFT		9
#define VPN_MASK		((1UL << VPN_SHIFT) - 1)

#define PTES_PER_PT	(PAGE_SIZE / sizeof(u64))
#define PAGE_SIZE		_BITUL(PAGE_SHIFT)
#define PAGE_MASK		PMASK(PAGE_SIZE)
#define PAGE_OFFS_MASK		(~PAGE_MASK)

#define MEGA_PAGE_SHIFT 	(PAGE_SHIFT + VPN_SHIFT)
#define MEGA_PAGE_SIZE		_BITUL(MEGA_PAGE_SHIFT)
#define MEGA_PAGE_MASK		PMASK(MEGA_PAGE_SIZE)
#define MEGA_PAGE_OFFS_MASK	(~MEGA_PAGE_MASK)

#define GIGA_PAGE_SHIFT 	(PAGE_SHIFT + 2 * VPN_SHIFT)
#define GIGA_PAGE_SIZE		_BITUL(GIGA_PAGE_SHIFT)
#define GIGA_PAGE_MASK		PMASK(GIGA_PAGE_SIZE)

#define PAGES(X)		((X) / PAGE_SIZE)
#define MEGA_PAGES(X)		((X) / MEGA_PAGE_SIZE)


#define INVALID_PHYS_ADDR	(~0UL)

#ifndef __ASSEMBLY__
struct paging {
        /** Page size of terminal entries in this level or 0 if none are
         * supported. */
        unsigned long page_size;

        pt_entry_t (*get_entry)(page_table_t page_table, unsigned long virt);
        bool (*entry_valid)(pt_entry_t pte, unsigned long flags);
        void (*set_terminal)(pt_entry_t pte, unsigned long phys,
                             unsigned long flags);
        unsigned long (*get_phys)(pt_entry_t pte, unsigned long virt);
        unsigned long (*get_flags)(pt_entry_t pte);
        void (*set_next_pt)(pt_entry_t pte, unsigned long next_pt);
        unsigned long (*get_next_pt)(pt_entry_t pte);
        void (*clear_entry)(pt_entry_t pte);
        bool (*page_table_empty)(page_table_t page_table);
};

static inline u64 page_up(u64 diff)
{
	return (diff + PAGE_SIZE - 1) & PAGE_MASK;
}

static inline u64 mega_page_up(u64 diff)
{
	return (diff + MEGA_PAGE_SIZE - 1) & MEGA_PAGE_MASK;
}

extern const struct paging *root_paging;

void arch_paging_init(void);
void arch_paging_enable(page_table_t pt);

int paging_init(void);
int paging_cpu_init(unsigned long hart_id);

/* Versatile mapper */
int map_range(page_table_t pt, const void *vaddr, paddr_t paddr, size_t size,
	      u16 flags);
int unmap_range(page_table_t pt, const void *vaddr, size_t size);

#endif /* __ASSEMBLY__ */

#include <asm/paging.h>

#endif /* _PAGING_H */
