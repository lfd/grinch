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

#ifndef _SYMBOLS_H
#define _SYMBOLS_H

#include <asm-generic/paging.h>
#include <grinch/compiler_attributes.h>

/* A region the linker script brackets with _start and _end symbols. */
#define LINKER_REGION_TYPED(name, type)	\
	extern type name##_start[], name##_end[]
#define LINKER_REGION(name)	LINKER_REGION_TYPED(name, unsigned char)

LINKER_REGION(__init_rw);
LINKER_REGION(__bootparams);
LINKER_REGION(__drivers);
LINKER_REGION(__irqchip_drivers);
LINKER_REGION(__pci_drivers);
LINKER_REGION(__rodata);
LINKER_REGION(__dtb);
LINKER_REGION(__rw_data);
LINKER_REGION(__bss);
LINKER_REGION(__internal_page_pool);

extern unsigned char __init_text_start[];
extern unsigned char __init_start[], __init_ro_end[];
/*
 * Try to avoid using __start in early boot context. For the absolute location,
 * always use grinch_base().
 */
extern unsigned char __start[], __text_end[];
/* Function pointers, walked as pointer-sized cells. */
LINKER_REGION_TYPED(__init_array, unsigned long);

static __always_inline size_t num_os_pages(void)
{
    return ((uintptr_t)__internal_page_pool_start -
	   (uintptr_t)__start) >> PAGE_SHIFT;
}

static __always_inline size_t internal_page_pool_pages(void)
{
	return ((uintptr_t)__internal_page_pool_end -
	        (uintptr_t)__internal_page_pool_start) >> PAGE_SHIFT;
}

#endif /* _SYMBOLS_H */
