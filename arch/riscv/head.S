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

#include <grinch/percpu.h>

.section .text, "ax"

.macro lpc reg, symbol
1:	auipc	\reg, %pcrel_hi (\symbol)
	addi	\reg, \reg, %pcrel_lo (1b)
.endm

.globl vmgrinch_start
vmgrinch_start:
	/*
	 * We arrive here with:
	 * a0: hartid
	 * a1: phys fdt
	 * a2: phys base of vmgrinch
	 */

	/*
	 * Setup stack: The stack for a HART is located at
	 * __internal_page_pool_end - hartid * STRUCT_PER_CPU_SIZE + STACK_SIZE
	 *
	 * From this address, subtract STRUCT_REGISTERS_SIZE. We will place
	 * user context here on return.
	 */
	la	t0, __internal_page_pool_end
	li	t1, STRUCT_PER_CPU_SIZE
	addi	t2, a0, 1
	mul	t2, t2, t1
	sub	sp, t0, t2
	li	t1, STACK_SIZE
	add	sp, sp, t1
	li	t1, STRUCT_REGISTERS_SIZE
	sub	sp, sp, t1

	/* push fdt location */
	addi	sp, sp, -8
	sd	a1, 0(sp) /* fdt */
	addi	sp, sp, -8
	sd	a0, 0(sp) /* hart-id */

	/* Clear BSS segment */
	la 	t0, __bss_start
	la 	t1, __bss_qwords

1:	sd	zero, 0(t0)
	addi	t0, t0, 8
	addi 	t1, t1, -1
	bne	t1, zero, 1b
	/* BSS cleared */

	/* set v2p for C code */
	mv	a0, a2
	jal	kmm_set_base

	/* Setup exception trap vector. Exception stack is yet uninitialised. */
	csrw	sscratch, zero
	la	t0, excp_handler
	csrw	stvec, t0

	/* pop parameters from stack */
	ld	a0, 0(sp) /* hart-id */
	addi	sp, sp, 8
	ld	a1, 0(sp) /* fdt */
	addi	sp, sp, 8

	/* Terminate stack frame */
	mv	s0, sp

	/* Hand over to C */
	jal	cmain
	j	return_to_user

.globl _cpu_halt
_cpu_halt:
	wfi
	j	_cpu_halt

.globl secondary_start
secondary_start:
	/*
	 * a0: hart_id
	 * a1: satp
	 */

	/* Activate the MMU */
	sfence.vma
	csrw	satp, a1
	sfence.vma

	la	t0, secondary_virt_entry
	jalr	t0

secondary_virt_entry:
	/* setup stack */
	li	sp, STACK_TOP
	addi	sp, sp, -STRUCT_REGISTERS_SIZE

	/* Terminate stack frame */
	mv	s0, sp

	/* Setup exception trap vector. Exception stack is yet uninitialised. */
	la	t0, excp_handler
	csrw	stvec, t0

	jal	secondary_cmain
	bne	a0, zero, _cpu_halt

	j	return_to_user
