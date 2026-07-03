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

.section .text, "ax"

.globl return_to_user
return_to_user:
	b .

.globl vectors
.align 11
vectors:
	.rept 16
	.align 7
	b .
	.endr
