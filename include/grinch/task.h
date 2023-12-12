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

struct process {
	struct mm mm;
};

struct task {
	struct list_head tasks;

	struct registers regs;
	pid_t pid;
	enum task_state state;

	struct process process;
};

struct task *task_alloc_new(void);

int task_from_fs(struct task *task, const char *pathname);

void task_activate(struct task *task);
void arch_process_activate(struct process *task);
void arch_task_restore(void);
void task_set_context(struct task *task, unsigned long pc, unsigned long sp);
void task_destroy(struct task *task);

void prepare_user_return(void);
void schedule(void);
void sched_enqueue(struct task *task);
void sched_dequeue(struct task *task);

int do_fork(void);
int do_execve(const char *pathname, char *const argv[], char *const envp[]);

#endif /* _TASK_H */
