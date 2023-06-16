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

#ifndef _SYMBOLS_H
#define _SYMBOLS_H

#include <grinch/compiler_attributes.h>

extern unsigned char __load_addr[], __text_end[];
extern unsigned char __rodata_start[], __rodata_end[];
extern unsigned char __rw_data_start[], __rw_data_end[];
extern unsigned char __internal_page_pool_start[];
extern unsigned char __internal_page_pool_pages[];
extern unsigned char __num_os_pages[];

#define _load_addr	((void *)&__load_addr)

#define SYMBOL_SZ(__sym)				\
	static __always_inline size_t __sym(void) {	\
		return (size_t)(uintptr_t)&__##__sym;	\
	}

SYMBOL_SZ(num_os_pages)
SYMBOL_SZ(internal_page_pool_pages)

/* Not available in guest code */
#define INC_BIN(__sym)						\
	extern unsigned char __##__sym[];			\
	extern unsigned char __##__sym##_end[];			\
								\
	static __always_inline void *_##__sym(void)		\
	{							\
		return (void*)&__##__sym;			\
	}							\
	static __always_inline size_t _##__sym##_size(void)	\
	{							\
		return __##__sym##_end - __##__sym;		\
	}

INC_BIN(guest_code)
INC_BIN(guest_dtb)

extern unsigned char excp_handler[];

#endif /* _SYMBOLS_H */
