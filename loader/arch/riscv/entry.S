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

#define RISCV_IMAGE_MAGIC	"RISCV\0\0\0"
#define RISCV_IMAGE_MAGIC2	"RSC\x05"

#define RISCV_HEADER_VERSION_MAJOR 0
#define RISCV_HEADER_VERSION_MINOR 2

#define RISCV_HEADER_VERSION	(RISCV_HEADER_VERSION_MAJOR << 16 | \
				 RISCV_HEADER_VERSION_MINOR)

.section .text.boot, "ax"

.macro lpc reg, symbol
1:	auipc	\reg, %pcrel_hi (\symbol)
	addi	\reg, \reg, %pcrel_lo (1b)
.endm

/*
 * This adds the Linux boot header. We're compatible with both: we're bootable
 * as simple SBI payload, and we're bootable via U-Boot.
 */
.globl _start
_start:
	j _loader_start
	.balign 8
	/* Image load offset. Load it to a 2M boundary. */
	.dword 0x200000

	.dword __stack_end - _start
	.dword 0
	.word RISCV_HEADER_VERSION
	.word 0
	.dword 0
	.ascii RISCV_IMAGE_MAGIC
	.balign 4
	.ascii RISCV_IMAGE_MAGIC2
	.word 0

_loader_start:
	lpc	sp, __stack_end

	/* Clear BSS segment */
	lpc     t0, __bss_start
	la      t1, __bss_qwords

1:	sd      zero, 0(t0)
	addi    t0, t0, 8
	addi    t1, t1, -1
	bne     t1, zero, 1b
	/* BSS cleared */

	/*
	 * a0: hartid <- already present
	 * a1: device-tree address <- already present
	 * a2: pc-rel load addr
	 */
	lpc	a2, _start
	jal	loader

1:
	wfi
	j 1b


.section ".grinch", "ax"
.incbin "vmgrinch.bin"
