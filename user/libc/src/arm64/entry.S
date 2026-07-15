/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2026
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
	ldr	x1, =__bss_start
	ldr	x2, =__bss_words

	cbz	w2, 2f
1:	str	xzr, [x1], #SZREG
	sub	w2, w2, #1
	cbnz	w2, 1b
	/* BSS cleared */

2:
	/* sp points to argc; pass it as cmain(long *p) expects */
	mov	x0, sp
	bl	cmain
	/* cmain is noreturn; loop defensively */
3:
	b	3b
