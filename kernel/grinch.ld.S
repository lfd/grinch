/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <arch/asm.h>
#include <asm-generic/grinch_layout.h>
#include <asm-generic/paging.h>

ENTRY(vmgrinch_start)
SECTIONS
{
	. = VMGRINCH_BASE;

	__load_addr = .;
	.text		: {
		KEEP(*(.header));
		. = ALIGN(16);
		*(.text);
		*(.text.*);
	}

	. = ALIGN(PAGE_SIZE);
	__init_text_start = .;
	.init.text	: {
		*(.init.text);
	}
	__text_end = .;

	. = ALIGN(PAGE_SIZE);
	__init_start = .;
	.init		: {
		__bootparams_start = .;
		KEEP(*(.init.bootparams));
		__bootparams_end = .;

		. = ALIGN(16);
		__drivers_start = .;
		KEEP(*(.drivers));
		__drivers_end = .;

		. = ALIGN(16);
		__pci_drivers_start = .;
		KEEP(*(.pci_drivers));
		__pci_drivers_end = .;

		. = ALIGN(16);
		*(.init.rodata);
		__init_ro_end = .;

		. = ALIGN(PAGE_SIZE);
		__init_rw_start = .;
		*(.init.data);
		__init_rw_end = .;

	}

	. = ALIGN(PAGE_SIZE);
	__rw_data_start = .;
	.data           : {
		*(.sdata)
		*(.data)
	}

	. = ALIGN(PAGE_SIZE);
	.bss            : {
		__bss_start = .;
		*(.bss)
		. = ALIGN(8);
		*(.bss.*)
		. = ALIGN(8);
		*(.sbss)
		. = ALIGN(8);
	}
	__bss_words = SIZEOF(.bss) / SZREG;
	__rw_data_end = .;

	. = ALIGN(PAGE_SIZE);
	__rodata_start = .;
	.rodata		: {
		*(.srodata.*)
		*(.rodata)
		*(.rodata.*)

		. = ALIGN(8);
		__init_array_start = .;
		KEEP(*(.init_array.*)) *(.init_array)
		__init_array_end = .;
	}
	__rodata_end = .;

	. = ALIGN(PAGE_SIZE);

	__internal_page_pool_start = .;
	__num_os_pages = (__internal_page_pool_start - __load_addr) / PAGE_SIZE;
	. = __load_addr + GRINCH_SIZE;
	__internal_page_pool_end = .;
	__internal_page_pool_pages =
		(__internal_page_pool_end - __internal_page_pool_start) / PAGE_SIZE;
}
