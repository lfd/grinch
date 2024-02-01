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

#define dbg_fmt(x)	"task: " x

#include <asm/spinlock.h>
#include <asm_generic/grinch_layout.h>

#include <grinch/alloc.h>
#include <grinch/arch.h>
#include <grinch/boot.h>
#include <grinch/errno.h>
#include <grinch/hypercall.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/percpu.h>

#define GRINCH_VM_PID_OFFSET	10000

static LIST_HEAD(task_list);
static DEFINE_SPINLOCK(task_lock);
static pid_t next_pid = 1;

void task_destroy(struct task *task)
{
	switch (task->type) {
	case GRINCH_PROCESS:
		process_destroy(task);
		break;

	case GRINCH_VMACHINE:
		vmachine_destroy(task);
		break;

	default:
		panic("Unknown task type\n");
		break;
	}

	/*
	 * Once we have proper SMP, this won't work like that any longer. We
	 * first have to make sure, that a Task got suspended before being
	 * dequeued
	 */
	if (!list_empty(&task->tasks))
		sched_dequeue(task);

	if (this_per_cpu()->current_task == task) {
		this_per_cpu()->schedule = true;
		this_per_cpu()->current_task = NULL;
	}

	kfree(task);
}

static inline pid_t get_new_pid(void)
{
	pid_t ret;

	spin_lock(&task_lock);
	ret = next_pid++;
	spin_unlock(&task_lock);

	return ret;
}

struct task *task_alloc_new(void)
{
	struct task *task;

	task = kzalloc(sizeof(*task));
	if (!task)
		return ERR_PTR(-ENOMEM);

	task->pid = get_new_pid();
	task->state = SUSPENDED;
	task->type = GRINCH_UNDEF;
	INIT_LIST_HEAD(&task->tasks);

	return task;
}

void task_activate(struct task *task)
{
	struct per_cpu *tpcpu;

	tpcpu = this_per_cpu();
	if (tpcpu->current_task && tpcpu->current_task->state == RUNNING) {
		tpcpu->current_task->state = SUSPENDED;
	}

	tpcpu->current_task = task;
	task->state = RUNNING;

	switch (task->type) {
	case GRINCH_PROCESS:
		arch_process_activate(task->process);
		break;

	case GRINCH_VMACHINE:
		arch_vmachine_activate(task->vmachine);
		break;

	default:
		panic("Unknown task type!\n");
		break;
	}
}

void sched_dequeue(struct task *task)
{
	spin_lock(&task_lock);
	list_del(&task->tasks);
	spin_unlock(&task_lock);
}

void sched_enqueue(struct task *task)
{
	spin_lock(&task_lock);
	list_add(&task->tasks, &task_list);
	spin_unlock(&task_lock);
}

void schedule(void)
{
	struct task *task;
	struct per_cpu *tpcpu;

	tpcpu = this_per_cpu();
	spin_lock(&task_lock);
	tpcpu->schedule = false;
	task = NULL;

	// Use this chance to allow other VMs to run
	if (grinch_is_guest)
		hypercall_yield();

	if (!tpcpu->current_task)
		goto begin;

	if (list_is_singular(&task_list)) {
		task = list_first_entry(&task_list, struct task, tasks);
		goto out;
	}

	task = tpcpu->current_task;
	list_for_each_entry_from(task, &task_list, tasks) {
		if (task->state == SUSPENDED)
			goto out;
	}

begin:
	if (list_empty(&task_list)) {
		this_per_cpu()->current_task = NULL;
		goto out;
	}

	list_for_each_entry(task, &task_list, tasks) {
		if (task == tpcpu->current_task)
			break;

		if (task->state == SUSPENDED)
			goto out;
	}

	task = NULL;

out:
	if (task)
		task_activate(task);
	spin_unlock(&task_lock);
}

int do_fork(void)
{
	struct task *this, *new;
	struct list_head *pos;
	struct vma *vma;
	int err;

	this = current_task();
	new = process_alloc_new();
	if (IS_ERR(new))
		return PTR_ERR(new);

	new->regs = this->regs;
	regs_set_retval(&new->regs, 0);

	list_for_each(pos, &this->process->mm.vmas) {
		vma = list_entry(pos, struct vma, vmas);
		err = uvma_duplicate(new->process, this->process, vma);
		if (err)
			goto destroy_out;
	}

	sched_enqueue(new);

	return new->pid;

destroy_out:
	task_destroy(new);
	return err;
}

void prepare_user_return(void)
{
	if (this_per_cpu()->schedule)
		schedule();

	if (!this_per_cpu()->current_task) {
		ps("Nothing to schedule!\n");
		arch_shutdown();
	}

	else if (this_per_cpu()->pt_needs_update) {
		arch_process_activate(current_task()->process);
		this_per_cpu()->pt_needs_update = false;
	}
	arch_task_restore();
}

int __init task_init(void)
{
	if (grinch_is_guest)
		next_pid += GRINCH_VM_PID_OFFSET * grinch_id;

	return 0;
}
