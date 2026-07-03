/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2025
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/task.h>

void arch_kinfo_init(struct kinfo *kinfo) {}

void task_set_context(struct task *task, unsigned long pc, unsigned long sp)
{
	/* IMPLEMENT ME! */
	for (;;);
}

void arch_process_activate(struct process *p) {}

void arch_process_deactivate(void) { /* IMPLEMENT ME! */ }

void arch_mm_init(struct mm *mm) {}
