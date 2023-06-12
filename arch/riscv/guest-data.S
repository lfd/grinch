/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

.section .rodata

.macro incbin name, filename
.globl \name
\name:
	.incbin "\filename"
.globl \name\()_end
\name\()_end:
.endm

#incbin __guest_code, "guest/guest.bin"
#incbin __guest_dtb, "guest.dtb"
