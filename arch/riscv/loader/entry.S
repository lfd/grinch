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

.section .text.boot, "ax"

.macro lpc reg, symbol
1:	auipc	\reg, %pcrel_hi (\symbol)
	addi	\reg, \reg, %pcrel_lo (1b)
.endm

.globl _loader_start
_loader_start:
	wfi
	j _loader_start

.section ".text.grinch"
.globl __vmgrinch_bin
__vmgrinch_bin:
.incbin "vmgrinch.bin"
.globl __vmgrinch_end
__vmgrinch_end:
