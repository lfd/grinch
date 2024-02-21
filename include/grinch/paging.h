/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _PAGING_H
#define _PAGING_H

#include <asm/paging_common.h>

#include <grinch/const.h>
#include <grinch/types.h>
#include <grinch/utils.h>

#define GRINCH_MEM_R		(1 << 0)
#define GRINCH_MEM_W		(1 << 1)
#define GRINCH_MEM_X		(1 << 2)
#define GRINCH_MEM_U		(1 << 3)
#define GRINCH_MEM_DEVICE	(1 << 4)

#define GRINCH_MEM_RW		(GRINCH_MEM_R | GRINCH_MEM_W)
#define GRINCH_MEM_RX		(GRINCH_MEM_R | GRINCH_MEM_X)
#define GRINCH_MEM_RWXU		(GRINCH_MEM_RW | GRINCH_MEM_X | GRINCH_MEM_U)
#define GRINCH_MEM_DEFAULT	GRINCH_MEM_RW

#define INVALID_PHYS_ADDR	(~0UL)

#ifndef __ASSEMBLY__
typedef unsigned char mem_flags_t;

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

extern const struct paging *root_paging;
extern const struct paging *vm_paging;

void arch_paging_init(void);
void arch_paging_enable(unsigned long this_cpu, page_table_t pt);

int paging_init(unsigned long this_cpu);
int paging_cpu_init(unsigned long this_cpu);
int paging_discard_init(void);

paddr_t paging_get_phys(page_table_t pt, const void *virt);

/* Versatile mapper */
int map_range(page_table_t pt, const void *vaddr, paddr_t paddr, size_t size,
	      mem_flags_t grinch_flags);
int unmap_range(page_table_t pt, const void *vaddr, size_t size);

/* Versatile VM mapper */
int vm_map_range(page_table_t pt, const void *vaddr, paddr_t paddr, size_t size,
		 mem_flags_t grinch_flags);
int vm_unmap_range(page_table_t pt, const void *vaddr, size_t size);

#endif /* __ASSEMBLY__ */

#include <asm/paging.h>

#endif /* _PAGING_H */
