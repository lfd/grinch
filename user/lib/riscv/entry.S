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

.section .text.boot, "ax"

.globl _start
_start:
	/* Clear BSS segment */
	la	t0, __bss_start
	la	t1, __bss_words

	beqz	t1, 2f
1:	REG_S	zero, 0(t0)
	addi	t0, t0, SZREG
	addi	t1, t1, -1
	bne	t1, zero, 1b
	/* BSS cleared */
2:

	/* pass the stack pointer: points to argc */
	mv	a0, sp

	jal	cmain
	/* should we ever return from cmain, loop */
3:
	j 3b
