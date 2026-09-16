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

#include <arch/asm.h>
#include <asm/asm_defines.h>

#include <grinch/percpu.h>

#define PHDRS_BSS
#include <asm-generic/elf.h>
#include <asm-generic/grinch_layout.h>
#include <asm-generic/paging.h>

ENTRY(__start)
SECTIONS
{
	. = GRINCH_BASE;

	__start = .;
	.text		: {
		KEEP(*(.header0));
		KEEP(*(.header1));
		. = ALIGN(16);
		*(.text);
		*(.text.*);
	} :text

	. = ALIGN(PAGE_SIZE);
	__init_text_start = .;
	.init.text	: {
		*(.init.text);
	} :text
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
		__irqchip_drivers_start = .;
		KEEP(*(.drivers.irqchip));
		__irqchip_drivers_end = .;

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

	} :data

	. = ALIGN(PAGE_SIZE);
	__rw_data_start = .;
	.data           : {
		*(.sdata)
		*(.sdata.*)
		*(.data)
		*(.data.*)
	} :data
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

		. = ALIGN(8);
		__dtb_start = .;
		KEEP(*(.dtb));
		__dtb_end = .;
	} :rodata
	__rodata_end = .;

	/*
	 * Last, so that it takes no room in the image file: whatever loads the
	 * file only carries what has content, and the kernel zeroes this
	 * itself on entry.
	 */
	. = ALIGN(PAGE_SIZE);
	.bss            : {
		__bss_start = .;
		*(.bss)
		. = ALIGN(8);
		*(.bss.*)
		. = ALIGN(8);
		*(.sbss)
		*(.sbss.*)
		. = ALIGN(8);
	} :bss
	__bss_words = SIZEOF(.bss) / SZREG;
	__bss_end = .;

	/* One block per possible CPU, right behind the image. */
	. = ALIGN(PAGE_SIZE);
	__percpu_start = .;
	. = . + MAX_CPUS * STRUCT_PER_CPU_SIZE;
	__percpu_end = .;

	. = ALIGN(PAGE_SIZE);

	__internal_page_pool_start = .;
	. = __start + GRINCH_SIZE;
	__internal_page_pool_end = .;
}
