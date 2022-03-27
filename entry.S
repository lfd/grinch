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
#include <grinch/percpu.h>
#include <grinch/grinch_layout.h>

.section .header, "ax"

.macro lpc reg, symbol
1:	auipc	\reg, %pcrel_hi (\symbol)
	addi	\reg, \reg, %pcrel_lo (1b)
.endm

.globl start
start:
	/*
	 * Setup stack: The stack for a HART is located at
	 * __internal_page_pool_end - hartid * STRUCT_PER_CPU_SIZE + STACK_SIZE
	 */
	lpc	t0, __internal_page_pool_end
	li	t1, STRUCT_PER_CPU_SIZE
	addi	t2, a0, 1
	mul	t2, t2, t1
	sub	sp, t0, t2
	li	t1, STACK_SIZE
	add	sp, sp, t1

	/* push fdt location */
	addi	sp, sp, -8
	sd	a1, 0(sp) /* fdt */

	/* load pc-relative load address (i.e., where we are) */
	lpc	a1, start
	/* load absolue link address (i.e., where we want to be) */
	ld	a2, ___load_addr
	jal	setup_trampoline
	/* a0 will now hold v2p offset */

	/* get environment under control */
	lpc	t0, __internal_page_pool_end
	sub	sp, t0, sp
	li	t0, PERCPU_BASE
	li	t1, STRUCT_PER_CPU_SIZE
	remu	sp, sp, t1
	add	t0, t0, t1
	sub	sp, t0, sp

	lpc	a1, virt_entry
	add	a1, a1, a0
	jalr	a1

virt_entry:
	/* Clear BSS segment */
	la 	t0, __bss_start
	la 	t1, __bss_qwords

1:	sd	zero, 0(t0)
	addi	t0, t0, 8
	addi 	t1, t1, -1
	bne	t1, zero, 1b
	/* BSS cleared */

	/* set v2p for C code */
	jal	mm_set_int_v2p_offset

	/* Setup stvec */
	li	t1, EXCEPTION_STACK_TOP
	csrw	sscratch, t1

	la	t0, excp_handler
	csrw	stvec, t0

	/* pop parameters from stack */
	ld	a0, 0(sp)
	addi	sp, sp, 8

	/* Hand over to C */
	jal	cmain
1:	wfi
	jal	1b

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

	/* setup exception trap vectors */
	li	t1, EXCEPTION_STACK_TOP
	csrw	sscratch, t1

	la	t0, excp_handler
	csrw	stvec, t0

	jal	secondary_cmain
1:	wfi
	j	1b

.globl ___load_addr
___load_addr:
	.quad	__load_addr
