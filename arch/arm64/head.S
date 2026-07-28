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

#include <arch/asm.h>
#include <asm/asm_defines.h>
#include <asm/sysregs.h>

#include <grinch/percpu.h>

#define ARM64_IMAGE_MAGIC	"ARM\x64"

.section .header0, "ax"
_start:
	b	_loader_start
	.balign	8
	.quad	0x200000 /* Image load offset. Mega Page size alignment  */

	.quad	__rodata_end - _start /* effective size of kernel image */
	.quad	0x0 /* flags */
	/* reserved */
	.quad	0
	.quad	0
	.quad	0
	.ascii	ARM64_IMAGE_MAGIC

.section .text, "ax"
_loader_start:
	/*
	 * Only kick off the first CPU. Let secondary CPUs halt. Park secondary
	 * CPUs in their current EL.
	 */
	mrs	x1, mpidr_el1
	and	x1, x1, #0xff
	cbnz	x1, _cpu_halt

	/* Drop to EL1 with the MMU off; x0 (the FDT pointer) is preserved. */
	bl	converge_to_el1

	/* Load pc-rel sp */
	adrp	x1, __internal_page_pool_end
	mov	sp, x1

	msr	daif, xzr

	/* x0 still holds the fdt address */
	adrp	x1, _start
	bl	loader

.globl grinch_start
grinch_start:
	/* Disable paging via TTBR0 */
	mrs	x3, TCR
	orr	x3, x3, #TCR_EL1_EPD0
	msr	TCR, x3

	/* Zero the boot CPU's per_cpu block, as it may be dirty */
	ldr	x2, =__internal_page_pool_end
	mov	x3, #STRUCT_PER_CPU_SIZE
	sub	x2, x2, x3
1:	str	xzr, [x2], #8
	subs	x3, x3, #8
	b.ne	1b

	bl	setup_percpu_stack

	/* Push Device Tree location on the stack */
	str	x1, [sp, #-8]!

	/* Clear bss segment */
	ldr	x2, =__bss_start
	ldr	w3, =__bss_words

1:
	cbz	w3, 2f
	str	xzr, [x2], #SZREG
	sub	w3, w3, #1
	cbnz	w3, 1b

	/* Setup vectors */
2:
	ldr	x2, =vectors
	msr	vbar_el1, x2

	/* x0 still holds the offset from the loader */
	bl	kmm_set_base

	/* Boot CPU is always 0 on ARM64 */
	mov	x0, #0
	/* Pop Device tree location from the stack */
	ldr	x1, [sp], #8

	bl	cmain
	b	return_to_user

.globl _cpu_halt
_cpu_halt:
	wfi
	b	_cpu_halt

/*
 * converge_to_el1 - drop this CPU to EL1 (MMU off) and enable FP/SIMD.
 *
 * bl'able from any EL: the EL3/EL2 arms eret down a level and land back in
 * this routine, so it *returns at EL1* regardless of entry EL. eret leaves
 * GPRs intact and the whole routine runs at the physical PC (MMU off), so
 * the bl return address in x30 stays valid across the erets. Touches only
 * x1/x2; x0 (the boot CPU's FDT pointer) is preserved.
 */
converge_to_el1:
	mrs	x1, CurrentEL
	lsr	x1, x1, #2
	cmp	x1, #1
	beq	.Lconv_el1
	cmp	x1, #2
	beq	.Lconv_el2
	cmp	x1, #3
	beq	.Lconv_el3
	/* Unknown EL: nothing sane we can do. */
	b	_cpu_halt

.Lconv_el3:	/* EL3 -> EL2 */
	mov	x2, #SCR_EL3_INIT
	msr	scr_el3, x2
	mov	x2, #SPSR_EL2H_MASKED
	msr	spsr_el3, x2
	adr	x2, .Lconv_el2
	msr	elr_el3, x2
	eret

.Lconv_el2:	/* EL2 -> EL1 */
	/* Route the physical timer/counter to EL1 and zero the offset. */
	mrs	x1, cnthctl_el2
	orr	x1, x1, #3
	msr	cnthctl_el2, x1
	msr	cntvoff_el2, xzr
	/* EL1 is AArch64; SWIO hard-wired on pi3. */
	mov	x1, #HCR_EL2_RW
	orr	x1, x1, #HCR_EL2_SWIO
	msr	hcr_el2, x1
	mrs	x1, hcr_el2
	/* Minimal EL1 SCTLR (MMU still off) before we drop down. */
	mov	x2, #(SCTLR_EL1_RES1 & 0xffff)
	movk	x2, #(SCTLR_EL1_RES1 >> 16), lsl #16
	msr	sctlr_el1, x2
	/* eret into EL1 with DAIF masked. */
	mov	x2, #SPSR_EL1T_MASKED
	msr	spsr_el2, x2
	adr	x2, .Lconv_el1
	msr	elr_el2, x2
	eret

.Lconv_el1:	/* At EL1, MMU still off. Allow FP/SIMD, then return at EL1. */
	mov	x1, #CPACR_EL1_FPEN
	msr	cpacr_el1, x1
	ret

/*
 * setup_percpu_stack - set SP and TPIDR_EL1 for this CPU and reserve a
 * register frame at the top of the stack. Uses no stack of its own (safe to
 * bl before a stack exists); clobbers x2-x5, preserves x0/x1/x30.
 *
 * Stacks grow down from __internal_page_pool_end, one STRUCT_PER_CPU_SIZE
 * block per CPU; the slot is mpidr[7:0] (== the logical cpuid assigned in
 * arch_platform_init).
 */
setup_percpu_stack:
	ldr	x2, =__internal_page_pool_end
	mov	x3, #STRUCT_PER_CPU_SIZE
	mrs	x4, mpidr_el1
	and	x4, x4, #0xff
	add	x4, x4, #1
	mul	x4, x4, x3
	sub	sp, x2, x4

	/* TPIDR_EL1 holds this CPU's per_cpu structure from here on. */
	mov	x5, sp
	msr	tpidr_el1, x5

	add	sp, sp, #STACK_SIZE
	sub	sp, sp, #STRUCT_REGISTERS_SIZE
	ret

/*
 * Secondary CPU entry.
 *
 * Reached from arch_boot_cpu() once the boot CPU releases this core. On
 * entry we run at the *physical* address of this code, with the MMU off,
 * at whatever EL firmware left the core in (EL1 on QEMU virt, but EL2/EL3
 * elsewhere). The job here is to converge to EL1, enable the MMU with the
 * configuration the boot CPU captured, and then continue in the kernel's
 * high virtual mapping just like the boot CPU does after loader().
 */
.globl secondary_start
secondary_start:
	/*
	 * Converge to EL1 with the MMU off, just like the boot CPU: we
	 * cannot assume PSCI dropped us straight into EL1.
	 */
	bl	converge_to_el1

	/*
	 * Unmask SError and debug, but keep IRQ/FIQ masked: the GIC CPU
	 * interface is not up yet, and secondary_cmain() is entered with
	 * interrupts masked (like riscv). do_idle() unmasks them later.
	 */
	msr	daif, xzr
	msr	daifset, #(DAIF_IRQ | DAIF_FIQ)

	/*
	 * Load the MMU configuration the boot CPU stashed for us in
	 * arch_smp_bringup_init() (struct secondary_mmu_config in smp.c). It
	 * lives in the kernel image; with the MMU off we address it via
	 * adrp/add, which is PC-relative and therefore resolves to its
	 * *physical* location. Load order matches the struct field order:
	 * ttbr0, ttbr1, mair, tcr, sctlr.
	 */
	adrp	x0, secondary_mmu
	add	x0, x0, #:lo12:secondary_mmu
	ldr	x1, [x0], #8		/* ttbr0: identity trampoline */
	msr	TTBR0, x1
	ldr	x1, [x0], #8		/* ttbr1: shared kernel_root */
	msr	TTBR1, x1
	ldr	x1, [x0], #8		/* mair */
	msr	MAIR, x1
	ldr	x1, [x0], #8		/* tcr */
	msr	TCR, x1
	ldr	x2, [x0]		/* sctlr (applied after the isb) */
	isb

	/*
	 * Turn the MMU on (the SCTLR snapshot already has M/C/I set). The PC
	 * is still physical, but TTBR0 identity-maps it, so execution
	 * continues uninterrupted across the enable.
	 */
	msr	SCTLR, x2
	isb

	/*
	 * Jump to the high virtual mapping. "ldr =4f" pulls 4f's link-time
	 * (virtual) address from the literal pool, and the branch lands us in
	 * the TTBR1 mapping; from here on we no longer need TTBR0.
	 */
	ldr	x0, =4f
	br	x0

4:	/* Running virtually now. Disable TTBR0 so the trampoline can't be hit. */
	mrs	x3, TCR
	orr	x3, x3, #TCR_EL1_EPD0
	msr	TCR, x3
	isb

	bl	setup_percpu_stack

	/* Exception vectors (shared table). */
	ldr	x2, =vectors
	msr	vbar_el1, x2

	/*
	 * Into C, then eret to EL0 like the boot CPU (see 'b return_to_user'
	 * after 'bl cmain' above). secondary_cmain() enters the scheduler.
	 */
	bl	secondary_cmain
	b	return_to_user
