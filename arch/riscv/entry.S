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
	csrrw	tp, CSR_SCRATCH, tp
	bnez	tp, save_user_ctx

	/*
	 * tp is zero: the trap came from the kernel, whose tp just went into
	 * scratch. Take it back; the nested frame goes below the interrupted
	 * sp, which we still run on.
	 */
	csrr	tp, CSR_SCRATCH
	addi	sp, sp, -STRUCT_REGISTERS_SIZE
	context REG_S

	/* t0 belongs to the frame now, so it is ours to use. */
	li	t0, 1
	REG_S	t0, REG_FROM_KERNEL(sp)
	addi	t0, sp, STRUCT_REGISTERS_SIZE
	REG_S	t0, REG_SP(sp)
	j	common_ctx

save_user_ctx:
	/*
	 * tp holds the per-CPU block, handed over by scratch, which holds the
	 * task's tp in exchange. Spill the task's sp and fetch the kernel
	 * stack out of the block.
	 */
	REG_S	sp, PCPU_USER_SP(tp)
	REG_L	sp, PCPU_KERNEL_SP(tp)
	addi	sp, sp, -STRUCT_REGISTERS_SIZE
	context REG_S
	REG_S	zero, REG_FROM_KERNEL(sp)

	/* The frame caught our tp; replace it and the sp with the task's. */
	csrr	t0, CSR_SCRATCH
	REG_S	t0, REG_TP(sp)
	REG_L	t0, PCPU_USER_SP(tp)
	REG_S	t0, REG_SP(sp)

common_ctx:
	csrr	a0, CSR_EPC
	REG_S	a0, REG_PC(sp)

	/* A nested trap must take the kernel path. */
	csrw	CSR_SCRATCH, zero

	/* Terminate stack frame */
	mv	s0, sp

	jal	check_panic

	/* Pass registers; after check_panic as a0 is caller-saved */
	mv	a0, sp

	la	ra, return_from_trap
	csrr	a1, CSR_CAUSE
	bge	a1, zero, 1f
	j	arch_handle_irq

1:
	j	arch_handle_exception

.globl return_from_trap
return_from_trap:
	/* restore the interrupted PC */
	REG_L	a0, REG_PC(sp)
	csrw	CSR_EPC, a0

	/*
	 * Only a frame that interrupted a context of its own has an sp to
	 * swap back in; a nested kernel frame stays on the stack it runs on.
	 */
	REG_L	a0, REG_FROM_KERNEL(sp)
	bnez	a0, return_to_kernel

return_to_user:
#ifdef CONFIG_RISCV_M_MODE
	/*
	 * A task on a hart without a user mode runs at the level we run in,
	 * where CSR_STATUS's interrupt enable governs. The kernel keeps that
	 * clear and xret hands the saved copy back, so such a task would never
	 * see a timer tick and never be preempted. Arm the saved copy instead.
	 *
	 * FIXME: this lets the task run with interrupts enabled at our own
	 * level, which is only tolerable because a task that shares our level
	 * is not separated from us to begin with.
	 */
	la	a0, riscv_have_umode
	lb	a0, 0(a0)
	bnez	a0, 1f
	li	a0, SR_PIE
	csrs	CSR_STATUS, a0
1:
#endif
	/*
	 * Re-arm the entry path: scratch hands over the per-CPU block, the
	 * block hands out the kernel stack, whose top the frame sits at.
	 * No preemption must happen from now on.
	 */
	addi	a0, sp, STRUCT_REGISTERS_SIZE
	REG_S	a0, PCPU_KERNEL_SP(tp)
	csrw	CSR_SCRATCH, tp

	context REG_L
	REG_L	sp, REG_SP(sp)

	xret

return_to_kernel:
	/* Kernel return: sp is already the kernel stack; scratch stays 0. */
	context REG_L
	addi	sp, sp, STRUCT_REGISTERS_SIZE

	xret
