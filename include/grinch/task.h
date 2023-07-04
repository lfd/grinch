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

#ifndef _TASK_H
#define _TASK_H

#include <asm/cpu.h>

#include <grinch/compiler_attributes.h>
#include <grinch/list.h>
#include <grinch/types.h>
#include <grinch/vma.h>

typedef int pid_t;

enum task_state {
	SUSPENDED = 0,
	RUNNING,
};

struct task {
	struct registers regs;
	struct mm mm;

	struct list_head tasks;

	pid_t pid;
	enum task_state state;
};

struct task *task_from_elf(void *elf);

void task_activate(struct task *task);
void arch_task_activate(struct task *task);
void task_set_context(struct task *task, unsigned long pc, unsigned long sp);

void schedule(void);
void sched_enqueue(struct task *task);

#endif /* _TASK_H */
