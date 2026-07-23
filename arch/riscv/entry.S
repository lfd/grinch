/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <arch/asm.h>
#include <asm/asm_defines.h>
#include <asm/csr.h>

#include <grinch/percpu.h>

.section .text, "ax"

.macro context cmd
	.irp reg 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
		\cmd    x\reg, SZREG*(\reg - 1)(sp)
	.endr
.endm

.align 4
.globl excp_handler
excp_handler:
	csrrw	sp, sscratch, sp
	bnez	sp, save_user_ctx

	/* sp is zero: we have an exception from kernel space. So restore sp. */
	csrr	sp, sscratch
	addi	sp, sp, -STRUCT_REGISTERS_SIZE
	context REG_S
	j	common_ctx

save_user_ctx:
	addi	sp, sp, -STRUCT_REGISTERS_SIZE
	context REG_S

	/*
	 * Reclaim tp; the user's copy was just saved. The register frame
	 * sits at the top of the per-CPU stack, which in turn is the
	 * first member of struct per_cpu.
	 */
	li	t0, STACK_SIZE - STRUCT_REGISTERS_SIZE
	sub	tp, sp, t0

common_ctx:
	csrr	a0, sepc
	REG_S	a0, REG_PC(sp)

	/* push SP */
	csrr	a0, sscratch
	REG_S	a0, REG_SP(sp)
	csrw	sscratch, zero

	/* Terminate stack frame */
	mv	s0, sp

	jal	check_panic

	/* Pass registers; after check_panic as a0 is caller-saved */
	mv	a0, sp

	la	ra, return_from_trap
	csrr	a1, scause
	bge	a1, zero, 1f
	j	arch_handle_irq

1:
	j	arch_handle_exception

.globl return_from_trap
return_from_trap:
	/* restore the interrupted PC */
	REG_L	a0, REG_PC(sp)
	csrw	sepc, a0

	/*
	 * Where do we return to? A trap taken from supervisor mode -- which
	 * happens when we take an interrupt while idling (arch_do_idle runs
	 * wfi with interrupts enabled) -- returns to the kernel: sp is already
	 * the kernel stack and sscratch stays 0, so skip the user-mode
	 * sp/sscratch swap.
	 */
	csrr	a0, sstatus
	andi	a0, a0, SR_SPP
	bnez	a0, return_to_kernel

return_to_user:
	/* restore user's SP, keep the kernel SP in sscratch */
	REG_L	a0, REG_SP(sp)
	/* No preemption must happen from now on */
	csrw	sscratch, a0

	context REG_L
	addi	sp, sp, STRUCT_REGISTERS_SIZE

	csrrw	sp, sscratch, sp

	sret

return_to_kernel:
	/* Supervisor return: sp is already the kernel stack; sscratch stays 0. */
	csrw	sscratch, zero

	context REG_L
	addi	sp, sp, STRUCT_REGISTERS_SIZE

	sret
