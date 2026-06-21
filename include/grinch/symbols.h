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

extern unsigned char __init_text_start[];
extern unsigned char __init_start[], __init_ro_end[];
extern unsigned char __init_rw_start[], __init_rw_end[];
extern unsigned char __bootparams_start[], __bootparams_end[];
extern unsigned char __drivers_start[], __drivers_end[];
extern unsigned char __pci_drivers_start[], __pci_drivers_end[];
/*
 * Try to avoid using __start in early boot context. For the absolute location,
 * always use grinch_base().
 */
extern unsigned char __start[], __text_end[];
extern unsigned long __init_array_start[], __init_array_end[];
extern unsigned char __rodata_start[], __rodata_end[];
extern unsigned char __rw_data_start[], __rw_data_end[];
extern unsigned char __internal_page_pool_start[];
extern unsigned char __internal_page_pool_end[];

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
