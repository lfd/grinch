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

#include <asm/cpu.h>
#include <grinch/printk.h>
#include <grinch/stackdump.h>

register unsigned long current_stack_pointer __asm__("sp");

/*
 * AArch64 frame record (AAPCS64): the prologue does
 *   stp x29, x30, [sp, #-16]!  ; push {saved_fp, saved_lr}
 *   mov x29, sp                 ; fp = &frame_record
 * so x29 points directly at the two saved values.
 */
struct stackframe {
	unsigned long fp;
	unsigned long lr;
};

void stackdump(void)
{
	unsigned long fp, sp, pc;
	unsigned int frame_no;
	struct stackframe *frame;

	fp = (unsigned long)__builtin_frame_address(0);
	pc = (unsigned long)stackdump;
	sp = current_stack_pointer;

	pr("=== Stackdump ===\n");
	for (frame_no = 0; fp && fp >= sp; frame_no++) {
		pr("%3u: PC: " REG_FMT " SP: " REG_FMT " FP: " REG_FMT "\n",
		   frame_no, pc, sp, fp);

		frame = (struct stackframe *)fp;
		sp = fp;
		fp = frame->fp;
		pc = frame->lr;
	}
}
