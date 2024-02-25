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

#include <asm-generic/paging.h>

ENTRY(_user_start)

SECTIONS
{
	. = 0x80000000;
	__start = .;
	.text : {
		KEEP(*(.text.boot))
		*(.text)
		*(.text.*)
	}

	.rodata : {
	    *(.rodata)
	}

	. = ALIGN(PAGE_SIZE);
	.data : {
	    *(.data)
	}

	.bss :
	{
		. = ALIGN(8);
		__bss_start = .;
		*(.bss)
		. = ALIGN(8);
		__bss_end = .;
	}
	__bss_qwords = SIZEOF(.bss) / 8;
	. = ALIGN(PAGE_SIZE);

	/DISCARD/ : {
		*(.comment)
		*(.note*)
		*(.eh_frame*)
	}
}
