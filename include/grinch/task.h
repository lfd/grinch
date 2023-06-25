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

#include <asm/cpu.h>

#include <grinch/compiler_attributes.h>
#include <grinch/list.h>
#include <grinch/types.h>
#include <grinch/vma.h>

struct task {
	struct registers regs;
	struct mm mm;
};

struct task *task_from_elf(void *elf);

void task_activate(void);
void task_set_context(struct task *task, unsigned long pc, unsigned long sp);
