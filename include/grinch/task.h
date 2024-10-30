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

#define TASK_NAME_LEN	16

enum task_state {
	TASK_INIT = 0,
	TASK_RUNNABLE, /* Scheduleable */
	TASK_WFE, /* Waits for Events */
	TASK_RUNNING,
	TASK_EXIT_DEAD,
};

enum task_wfe {
	WFE_NONE = 0,
	WFE_CHILD,
	WFE_TIMER,
	WFE_READ,
};

enum task_type {
	GRINCH_UNDEF = 0,
	GRINCH_PROCESS,
	GRINCH_VMACHINE,
};

struct wfe_child {
	pid_t pid;
	int __user *status;
};

struct wfe_timer {
	unsigned long long expiration;
};

/* Blocking read from file descriptor */
struct wfe_read {
	struct file_handle *fh;
	char __user *ubuf;
	size_t count;
};

struct task {
	struct list_head tasks;
	spinlock_t lock;
	char name[TASK_NAME_LEN];

	struct registers regs;
	pid_t pid;
	enum task_state state;
	unsigned long on_cpu; /* only valid if state == TASK_RUNNING */

	struct task *parent;
	struct list_head children;
	/* working on siblings always requires the parent's lock */
	struct list_head sibling;
	struct list_head timer_list;

	struct {
		enum task_wfe type;

		union {
			struct wfe_child child;
			struct wfe_timer timer;
			struct wfe_read read;
		};
	} wfe; /* wait for event */

	int exit_code;

	enum task_type type;
	union {
		struct process process;
		struct vmachine vmachine;
	};
};

static __always_inline struct task *current_task(void)
{
	return this_per_cpu()->current_task;
}

static inline struct process *current_process(void)
{
	struct task *cur;

	cur = current_task();
	if (!cur)
		BUG();

	if (cur->type != GRINCH_PROCESS)
		BUG();

	return &cur->process;
}

static __always_inline struct file *cwd(void)
{
	return current_process()->cwd.file;
}

extern struct task *init_task;

struct task *task_alloc_new(const char *name);

void task_set_context(struct task *task, unsigned long pc, unsigned long sp);
void task_destroy(struct task *task);
void task_exit(struct task *task, int code);
void task_handle_events(void);
void task_save(struct registers *regs);

void task_handle_fault(void __user *addr, bool is_write);

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

/* utilities */
void task_set_name(struct task *task, const char *src);
void tasks_dump(void);

#endif /* _TASK_H */
