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

#include <asm/asm_defines.h>

.section .text, "ax"
.macro ventry label
	.align 7
	b       \label
.endm

.macro handler label
	.align 7
	sub sp, sp, #STRUCT_REGISTERS_SIZE
	stp x0, x1, [sp, #(0 * 16)]
	stp x2, x3, [sp, #(1 * 16)]
	stp x4, x5, [sp, #(2 * 16)]
	stp x6, x7, [sp, #(3 * 16)]
	stp x8, x9, [sp, #(4 * 16)]
	stp x10, x11, [sp, #(5 * 16)]
	stp x12, x13, [sp, #(6 * 16)]
	stp x14, x15, [sp, #(7 * 16)]
	stp x16, x17, [sp, #(8 * 16)]
	stp x18, x19, [sp, #(9 * 16)]
	stp x20, x21, [sp, #(10 * 16)]
	stp x22, x23, [sp, #(11 * 16)]
	stp x24, x25, [sp, #(12 * 16)]
	stp x26, x27, [sp, #(13 * 16)]
	stp x28, x29, [sp, #(14 * 16)]
	str x30, [sp, #(15 * 16)]

	mov	x0, sp
	bl	\label
	b	__return
.endm

.globl return_to_user
return_to_user:
__return:
	ldp x0, x1, [sp, #(REG_SP)]
	msr SP_EL0, x0
	msr ELR_EL1, x1

	ldr x2, [sp, #(REG_SPSR)]
	msr spsr_el1, x2

	ldp x0, x1, [sp, #(0 * 16)]
	ldp x2, x3, [sp, #(1 * 16)]
	ldp x4, x5, [sp, #(2 * 16)]
	ldp x6, x7, [sp, #(3 * 16)]
	ldp x8, x9, [sp, #(4 * 16)]
	ldp x10, x11, [sp, #(5 * 16)]
	ldp x12, x13, [sp, #(6 * 16)]
	ldp x14, x15, [sp, #(7 * 16)]
	ldp x16, x17, [sp, #(8 * 16)]
	ldp x18, x19, [sp, #(9 * 16)]
	ldp x20, x21, [sp, #(10 * 16)]
	ldp x22, x23, [sp, #(11 * 16)]
	ldp x24, x25, [sp, #(12 * 16)]
	ldp x26, x27, [sp, #(13 * 16)]
	ldp x28, x29, [sp, #(14 * 16)]
	ldr x30, [sp, #(15 * 16)]
	add sp, sp, #STRUCT_REGISTERS_SIZE

        eret
	/*
	 * Mitigate Straight-line Speculation.
	 * Guard against Speculating past an ERET instruction and
	 * potentially perform speculative accesses to memory before
	 * processing the exception return
	 */
	dsb nsh
	isb

.globl vectors
.align 11
vectors:
	/* Current EL with SP0 */
	ventry . /* synchronous */
	ventry . /* IRQ */
	ventry . /* FIQ */
	ventry . /* SError */

	/* Current EL with SPx */
	handler arch_handle_exception /* synchronous */
	handler arch_handle_irq /* IRQ */
	ventry .
	ventry .

	/* Lower EL using AArch64 */
	handler arch_handle_exception /* synchronous */
	handler arch_handle_irq /* IRQ */
	ventry .
	ventry .

	/* Lower EL using AArch32 */
	ventry .
	ventry .
	ventry .
	ventry .
