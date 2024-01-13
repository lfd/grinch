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
#include <grinch/process.h>
#include <grinch/types.h>

#include <grinch/arch/vmm.h>

typedef int pid_t;

enum task_state {
	TASK_RUNNABLE = 0, /* Scheduleable */
	TASK_WFE, /* Waits for Events */
	TASK_RUNNING,
};

enum task_type {
	GRINCH_UNDEF = 0,
	GRINCH_PROCESS,
	GRINCH_VMACHINE,
};

struct task {
	struct list_head tasks;

	struct registers regs;
	pid_t pid;
	enum task_state state;

	unsigned long long timer_expiration;

	enum task_type type;
	union {
		struct process *process;
		struct vmachine *vmachine;
	};
};

struct task *task_alloc_new(void);

void task_activate(struct task *task);
void arch_task_restore(void);
void task_set_context(struct task *task, unsigned long pc, unsigned long sp);
void task_destroy(struct task *task);
void task_handle_events(void);
void task_sleep_for(struct task *task, unsigned long long ns);

int task_init(void);

void prepare_user_return(void);
void sched_enqueue(struct task *task);
void sched_dequeue(struct task *task);

int do_fork(void);
int do_execve(const char *pathname, char *const argv[], char *const envp[]);

#endif /* _TASK_H */
