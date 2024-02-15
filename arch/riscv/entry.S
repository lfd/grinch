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

#include <asm/asm_defines.h>

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
	bnez	sp, save_ctx

	/* sp is zero: we have an exception from kernel space. So restore sp. */
	csrr	sp, sscratch

save_ctx:
	addi	sp, sp, -STRUCT_REGISTERS_SIZE
	context sd

	csrr	a0, sepc
	sd	a0, REG_PC(sp)

	/* push SP */
	csrr	a0, sscratch
	sd	a0, REG_SP(sp)
	csrw	sscratch, zero

	/* Pass registers */
	mv	a0, sp

	jal	check_panic

	la	ra, return_to_user
	csrr	a1, scause
	bge	a1, zero, 1f
	j	arch_handle_irq

1:
	j	arch_handle_exception

.globl return_to_user
return_to_user:
	/* restore user's PC */
	ld	a0, REG_PC(sp)
	csrw	sepc, a0

	/* restore user's SP, keep it in sscratch */
	ld	a0, REG_SP(sp)
	/* No preemption must happen from now on */
	csrw	sscratch, a0

	context ld
	addi	sp, sp, STRUCT_REGISTERS_SIZE

	csrrw	sp, sscratch, sp

	sret
