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

#include <asm/asm_defines.h>

.section .text, "ax"

.macro context cmd
	.irp reg 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
		\cmd    x\reg, 8*(\reg - 1)(sp)
	.endr
.endm

/* Here we arrive, if we have a fault from userspace. */
.align 4
.globl excp_handler_user
excp_handler_user:
	csrrw	sp, sscratch, sp
	j	excp_handler

/* Here we arrive directly, if we have a fault inside the kernel */
.align 4
.globl excp_handler
excp_handler:
	addi	sp, sp, -STRUCT_REGISTERS_SIZE
	context sd

	/* We're now in kernel space. So set the kernel's exception handler */
	la	t0, excp_handler
	csrw	stvec, t0

	/* push SP */
	csrr	a0, sscratch
	sd	a0, 8(sp)

	/* Pass registers */
	mv	a0, sp

	jal arch_handle_trap

.globl return_to_user
return_to_user:

	/* We're about to return to user. So set the user's exception handler */
	la	t0, excp_handler_user
	csrw	stvec, t0

	context ld
	addi	sp, sp, STRUCT_REGISTERS_SIZE

	csrrw	sp, sscratch, sp

	sret
