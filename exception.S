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

#include <grinch/asm-defines.h>

.section .text, "ax"

.macro context cmd
	.irp reg 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
		\cmd    x\reg, 8*(\reg - 1)(sp)
	.endr
.endm

.align 4
.globl excp_handler
excp_handler:
	csrrw	sp, sscratch, sp

	addi	sp, sp, -STRUCT_REGISTERS_SIZE
	context sd

	/* push SP */
	csrr	a0, sscratch
	sd	a0, 8(sp)

	/* Pass registers */
	mv	a0, sp

	jal arch_handle_trap

	context ld
	addi	sp, sp, STRUCT_REGISTERS_SIZE

	csrrw	sp, sscratch, sp

	sret
