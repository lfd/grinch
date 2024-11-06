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

#define LOADER_BASE	_UL(0x40000000)

/* Check if this applies for ARM64 */
#define USER_START		_UL(0x1000)

#define USER_STACK_SIZE		(1 * MIB)
#define USER_STACK_TOP		USER_END
#define USER_STACK_BOTTOM	(USER_STACK_TOP - USER_STACK_SIZE)

/* Must be a multiple of 256 KiB */
#define GRINCH_SIZE	(8 * 256 * KIB)

/* Generic definitions */
#define VMGRINCH_END	(VMGRINCH_BASE + GRINCH_SIZE)
#define IOREMAP_END	(IOREMAP_BASE + IOREMAP_SIZE)

#if ARCH_RISCV == 64 /* rv64 */
/*
 * On RV64 we have at least SV39 for paging. On SV39, this is the uppermost
 * address that comes before VMGRINCH_BASE
 */
#define VMGRINCH_BASE	_UL(0xffffffc000000000)
#define DIR_PHYS_BASE	(VMGRINCH_BASE + 1 * GIB)
#define PERCPU_BASE	(VMGRINCH_BASE + 64UL * 1 * GIB)

#define IOREMAP_BASE	(VMGRINCH_BASE + 256 * MIB)
#define IOREMAP_SIZE	(512 * MIB)
#define KHEAP_BASE      (IOREMAP_END)

#define USER_END		(_UL(1) << (39 - 1))

#elif ARCH_RISCV == 32 /* rv32 */
#define VMGRINCH_BASE	_UL(0xd8000000)
#define USER_END	IOREMAP_BASE
#define IOREMAP_BASE	_UL(0xc0000000)
#define IOREMAP_SIZE	((256 + 128) * MIB)

#define PERCPU_BASE	VMGRINCH_END
#define KHEAP_BASE      (VMGRINCH_END + (32 * 64) * KIB)

/* The uppermost 512 MiB belong to the direct mapping */
#define DIR_PHYS_BASE	_UL(0xe0000000)
#endif
