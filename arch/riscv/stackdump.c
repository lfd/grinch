/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2025
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

struct stackframe {
	unsigned long fp;
	unsigned long ra;
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
	for (frame_no = 0; ; frame_no++) {
		pr("%3u: PC: " REG_FMT " SP: " REG_FMT " FP: " REG_FMT "\n",
		   frame_no, pc, sp, fp);

		if (fp == sp)
			break;

		frame = (struct stackframe *)fp - 1;
		sp = fp;

		fp = frame->fp;
		pc = frame->ra;
	}
}
