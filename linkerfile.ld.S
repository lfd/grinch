/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/grinch_layout.h>

SECTIONS
{
	. = LINK_BASE;
	__load_addr = .;
	.header		: { *(.header) }
	.text		: {
		*(.text)
		*(.text.*)
	}
	__text_end = .;

	. = ALIGN(_PAGE_SIZE);
	__rw_data_start = .;
	.data           : {
		*(.sdata)
		*(.data)
	}
	
	. = ALIGN(_PAGE_SIZE);
	.bss            : {
		__bss_start = .;
		*(.bss)
		. = ALIGN(8);
		*(.sbss)
		. = ALIGN(8);
	}
	__bss_qwords = SIZEOF(.bss) / 8;
	__rw_data_end = .;

	. = ALIGN(_PAGE_SIZE);
	__rodata_start = .;
	.rodata		: {
		*(.srodata.*)
		*(.rodata)
		*(.rodata.*)
	}
	__rodata_end = .;

	. = ALIGN(_PAGE_SIZE);

	__internal_page_pool_start = .;
	__num_os_pages = (__internal_page_pool_start - __load_addr) / _PAGE_SIZE;
	. = __load_addr + GRINCH_SIZE;
	__internal_page_pool_end = .;
	__internal_page_pool_pages =
		(__internal_page_pool_end - __internal_page_pool_start) / _PAGE_SIZE;
}
