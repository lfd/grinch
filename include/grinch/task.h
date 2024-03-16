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

#ifndef _TASK_H
#define _TASK_H

#include <asm/cpu.h>

#include <grinch/compiler_attributes.h>
#include <grinch/list.h>
#include <grinch/panic.h>
#include <grinch/process.h>
#include <grinch/types.h>
#include <grinch/timer.h>

#include <grinch/arch/vmm.h>

enum task_state {
	TASK_INIT = 0,
	TASK_RUNNABLE, /* Scheduleable */
	TASK_WFE, /* Waits for Events */
	TASK_RUNNING,
	TASK_WAIT, /* Wait for children */
	TASK_EXIT_DEAD,
};

enum task_type {
	GRINCH_UNDEF = 0,
	GRINCH_PROCESS,
	GRINCH_VMACHINE,
};

struct task {
	struct list_head tasks;
	spinlock_t lock;

	struct registers regs;
	pid_t pid;
	enum task_state state;
	unsigned long on_cpu; /* only valid if state == TASK_RUNNING */

	struct task *parent;
	struct list_head children;
	/* working on siblings always requires the parent's lock */
	struct list_head sibling;
	struct {
		bool waiting;
		pid_t pid;
		int __user *status;
	} wait_for; /* wait for child */

	int exit_code;

	struct {
		struct list_head timer_list;
		unsigned long long expiration;
	} timer;

	enum task_type type;
	union {
		struct process *process;
		struct vmachine *vmachine;
	};
};

static inline struct task *current_task(void)
{
	return this_per_cpu()->current_task;
}

static inline struct process *current_process(void)
{
	struct task *cur;

	cur = current_task();
	if (cur->type != GRINCH_PROCESS)
		BUG();

	return cur->process;
}

struct task *task_alloc_new(void);

void task_set_context(struct task *task, unsigned long pc, unsigned long sp);
void task_destroy(struct task *task);
void task_exit(int code);
long task_wait(pid_t pid, int __user *wstatus, int options);
void task_handle_events(void);
void task_save(struct registers *regs);

void task_set_wfe(struct task *task);
void task_sleep_until(struct task *task, unsigned long long wall_ns);
static inline void task_sleep_for(struct task *task, unsigned long long ns)
{
	task_sleep_until(task, timer_get_wall() + ns);
}
void task_cancel_timer(struct task *task);

void arch_vmachine_save(struct vmachine *vm);
void arch_vmachine_restore(struct vmachine *vm);

int task_init(void);

void prepare_user_return(void);
void sched_enqueue(struct task *task);
void sched_dequeue(struct task *task);

/* invoke scheduler on all CPUs */
void sched_all(void);

int do_fork(void);

#endif /* _TASK_H */
