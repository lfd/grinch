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

#include <asm_generic/grinch_layout.h>

.section .text.boot, "ax"

.macro lpc reg, symbol
1:	auipc	\reg, %pcrel_hi (\symbol)
	addi	\reg, \reg, %pcrel_lo (1b)
.endm

.globl _loader_start
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
	lpc	a2, _loader_start
	/* a3: abs link addr */
	li	a3, VMGRINCH_BASE
	jal	loader

1:
	wfi
	j 1b


.section ".grinch", "ax"
.incbin "vmgrinch.bin"
