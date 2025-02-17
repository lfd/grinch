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

#include <arch/asm.h>
#include <asm-generic/paging.h>

ENTRY(_start)

SECTIONS
{
	. = 0x90000000;
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

	. = ALIGN(16);
	.bss :
	{
		__bss_start = .;
		*(.bss)
		. = ALIGN(16);
		__bss_end = .;
	}
	__bss_words = SIZEOF(.bss) / SZREG;
	. = ALIGN(PAGE_SIZE);

	/DISCARD/ : {
		*(.comment)
		*(.note*)
		*(.eh_frame*)
	}
}
