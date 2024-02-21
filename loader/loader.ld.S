/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm_generic/grinch_layout.h>
#include <asm/paging_common.h>

ENTRY(_loader_start)

SECTIONS
{
	. = LOADER_BASE;
	__start = .;
	.text : {
		KEEP(*(.text.boot))
		*(.text)
		*(.text.*)
	}

	.rodata : {
	    *(.rodata)
	}

	.grinch : {
		__vmgrinch_bin_start = .;
		KEEP(*(.grinch))
		__vmgrinch_bin_end = .;
	}
	__vmgrinch_bin_size = __vmgrinch_bin_end - __vmgrinch_bin_start;

	. = ALIGN(PAGE_SIZE);
	.data : {
	    *(.data)
	}

	.bss :
	{
		. = ALIGN(16);
		__bss_start = .;
		*(.bss)
		. = ALIGN(16);
		__bss_end = .;
	}
	__bss_qwords = SIZEOF(.bss) / 8;
	. = ALIGN(PAGE_SIZE);
	__stack_start = .;
	. = . + PAGE_SIZE;
	__stack_end = .;

	/DISCARD/ : {
		*(.comment)
		*(.note*)
		*(.eh_frame*)
	}
}
