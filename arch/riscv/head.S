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

#include <arch/asm.h>
#include <asm/asm_defines.h>

#include <grinch/percpu.h>

#define RISCV_IMAGE_MAGIC	"RISCV\0\0\0"
#define RISCV_IMAGE_MAGIC2	"RSC\x05"

#define RISCV_HEADER_VERSION_MAJOR 0
#define RISCV_HEADER_VERSION_MINOR 2

#define RISCV_HEADER_VERSION	(RISCV_HEADER_VERSION_MAJOR << 16 | \
				 RISCV_HEADER_VERSION_MINOR)

.macro lpc reg, symbol
1:	auipc	\reg, %pcrel_hi (\symbol)
	addi	\reg, \reg, %pcrel_lo (1b)
.endm

/*
 * This adds the Linux boot header. We're compatible with both: we're bootable
 * as simple SBI payload, and we're bootable via U-Boot.
 */

.section .header0, "ax"
	j _loader_start
	.balign 8
	/*
	 * Image load offset. Load it to a 2M boundary on RV64, and a 4M
	 * boundary on RV32.
	 */
	.dword MEGA_PAGE_SIZE

	.dword __rodata_end - __start
	.dword 0
	.word RISCV_HEADER_VERSION
	.word 0
	.dword 0
	.ascii RISCV_IMAGE_MAGIC
	.balign 4
	.ascii RISCV_IMAGE_MAGIC2

.section .text, "ax"
_loader_start:
	/*
	 * a0: hartid <- already present
	 * a1: device-tree address <- already present
	 * a2: pc-rel load addr
	 */
	lpc	sp, __internal_page_pool_end
	lpc	a2, __start
	jal	loader

1:
	wfi
	j 1b

/* grinch startup code */
.globl grinch_start
grinch_start:
	/*
	 * We arrive here with:
	 * a0: hartid
	 * a1: phys fdt
	 * a2: phys base of grinch
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
	REG_S	a1, 0(sp) /* fdt */
	addi	sp, sp, -8
	REG_S	a0, 0(sp) /* hart-id */

	/* Clear BSS segment */
	la 	t0, __bss_start
	la 	t1, __bss_words

1:	REG_S	zero, 0(t0)
	addi	t0, t0, SZREG
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
	REG_L	a0, 0(sp) /* hart-id */
	addi	sp, sp, 8
	REG_L	a1, 0(sp) /* fdt */
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
