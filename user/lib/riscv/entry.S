/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

.section .text.boot, "ax"

.globl _user_start
_user_start:
	/* Clear BSS segment */
	la	t0, __bss_start
	la	t1, __bss_qwords

	beq	t1, zero, 2f
1:	sd	zero, 0(t0)
	addi	t0, t0, 8
	addi	t1, t1, -1
	bne	t1, zero, 1b
	/* BSS cleared */

2:
	jal	main
	jal	exit
	/* should we ever return from exit, loop */
3:
	j 3b
