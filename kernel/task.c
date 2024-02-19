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

#define dbg_fmt(x)	"task: " x

#include <asm/irq.h>
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
#include <grinch/timer.h>

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
	task->state = TASK_RUNNABLE;
	task->type = GRINCH_UNDEF;
	INIT_LIST_HEAD(&task->tasks);

	return task;
}

static void task_activate(struct task *task)
{
	struct per_cpu *tpcpu;
	struct task *old;

	old = current_task();
	if (old == task)
		return;

	tpcpu = this_per_cpu();
	/*
	 * Only set the task to runnable, if it was running before.
	 * Through a syscall, it might be set to wait for events.
	 */
	if (old && old->state == TASK_RUNNING)
		tpcpu->current_task->state = TASK_RUNNABLE;

	tpcpu->current_task = task;
	if (!task)
		return;

	if (task->state != TASK_RUNNABLE)
		panic("Activating non-runnable task: PID %u state: %x\n",
		      task->pid, task->state);
	task->state = TASK_RUNNING;

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

static void schedule(void)
{
	struct task *task;
	struct per_cpu *tpcpu;

	tpcpu = this_per_cpu();
	spin_lock(&task_lock);
	tpcpu->schedule = false;
	task = NULL;

#if 0
	// Use this chance to allow other VMs to run
	if (grinch_is_guest)
		hypercall_yield();
#endif

	if (!tpcpu->current_task)
		goto begin;

	if (list_is_singular(&task_list)) {
		task = list_first_entry(&task_list, struct task, tasks);
		if (task->state != TASK_RUNNABLE)
			task = NULL;
		goto out;
	}

	task = tpcpu->current_task;
	list_for_each_entry_from(task, &task_list, tasks) {
		if (task->state == TASK_RUNNABLE)
			goto out;
	}

begin:
	if (list_empty(&task_list))
		goto nothing;

	list_for_each_entry(task, &task_list, tasks) {
		if (task == tpcpu->current_task)
			break;

		if (task->state == TASK_RUNNABLE)
			goto out;
	}

nothing:
	/*
	 * We have nothing to schedule. But is the current task running and may
	 * continue?
	 */
	if (current_task() && current_task()->state == TASK_RUNNING)
		task = current_task();
	else
		task = NULL;

out:
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
	sched_all();

	return new->pid;

destroy_out:
	task_destroy(new);
	return err;
}

static inline void _task_set_wfe(struct task *task)
{
	task->state = TASK_WFE;
}

void task_set_wfe(struct task *task)
{
	spin_lock(&task_lock);
	_task_set_wfe(task);
	spin_unlock(&task_lock);
}

void task_sleep_until(struct task *task, unsigned long long wall_ns)
{
	spin_lock(&task_lock);
	/* VMs remain runnable */
	if (task->type == GRINCH_PROCESS)
		_task_set_wfe(task);

	task->timer.expiration = wall_ns;
	task->timer.active = true;
	spin_unlock(&task_lock);
}

void task_handle_events(void)
{

	struct task *task;

	spin_lock(&task_lock);
	list_for_each_entry(task, &task_list, tasks) {
		if (!task->timer.active)
			continue;

		/* we hit a sleeping task */
		if (timer_get_wall() >= task->timer.expiration) {
			if (task->state != TASK_RUNNING)
				task->state = TASK_RUNNABLE;
			task->timer.active = false;

			if (task->type == GRINCH_VMACHINE)
				arch_vmachine_inject_timer(task->vmachine);
		}
	}
	spin_unlock(&task_lock);
}

static void do_idle(void)
{
	this_per_cpu()->idling = true;
	irq_enable();
	cpu_do_idle();
	irq_disable();

	/* We might have received an IPI, so check events */
	check_events();

	this_per_cpu()->idling = false;
	mb();
}

static void task_restore(void)
{
	struct task *task = current_task();

	this_per_cpu()->stack.regs = task->regs;
	if (task->type == GRINCH_VMACHINE)
		arch_vmachine_restore(task->vmachine);
}

void task_save(struct registers *regs)
{
	struct task *task;

	task = current_task();
	/* Save task context */
	task->regs = *regs;
	if (task->type == GRINCH_VMACHINE)
		arch_vmachine_save(task->vmachine);
}

#if 0
static const char *task_type_to_string(enum task_type type)
{
	switch (type) {
		case GRINCH_UNDEF:
			return "undef  ";
		case GRINCH_PROCESS:
			return "process";
		case GRINCH_VMACHINE:
			return "VM     ";
		default:
			return "unknown";
	};
}

static const char *task_state_to_string(enum task_state state)
{
	switch (state) {
		case TASK_RUNNABLE:
			return "runnable";
		case TASK_RUNNING:
			return "running ";
		case TASK_WFE:
			return "wfe     ";
		default:
			return "unkn    ";
	}
}

static void dump_tasks(void)
{
	struct task *task;
	const char *timer_str;

	spin_lock(&task_lock);

	list_for_each_entry(task, &task_list, tasks) {
		if (list_empty(&task->timer.timer_list))
			timer_str = "not set";
		else
			timer_str = "active ";
		pr("PID: %u Type: %s State: %s On CPU: %lu Timer: %s "
		   "Expiration: " PR_TIME_FMT "\n",
		   task->pid, task_type_to_string(task->type),
		   task_state_to_string(task->state), task->on_cpu, timer_str,
		   PR_TIME_PARAMS(task->timer.expiration));
	}

	spin_unlock(&task_lock);
}

static void dump_timers(void)
{
	struct task *task;

	spin_lock(&task_lock);

	list_for_each_entry(task, &timer_list, timer.timer_list)
		pr("PID: %u, Expiration: " PR_TIME_FMT "\n", task->pid,
		   PR_TIME_PARAMS(task->timer.expiration));

	spin_unlock(&task_lock);
}
#endif

void prepare_user_return(void)
{
	struct per_cpu *tpcpu;

	tpcpu = this_per_cpu();
retry:
	if (tpcpu->schedule)
		schedule();

	if (!tpcpu->current_task) {
		if (list_empty(&task_list)) {
			if (tpcpu->primary) {
				ps("Nothing to schedule!\n");
				arch_shutdown(-ENOENT);
			}
		} else {
			if (tpcpu->idling)
				panic("Double idling.\n");
		}
		do_idle();
		goto retry;
	}

	if (tpcpu->pt_needs_update) {
		tpcpu->pt_needs_update = false;
		switch (current_task()->type) {
			case GRINCH_PROCESS:
				arch_process_activate(current_task()->process);
				break;
			default:
				panic("Not implemented\n");
				break;
		}
	}

	task_restore();
}

void sched_all(void)
{
	unsigned long cpu;

	for_each_online_cpu(cpu)
		per_cpu(cpu)->schedule = true;
	ipi_broadcast();
}

int __init task_init(void)
{
	if (grinch_is_guest)
		next_pid += GRINCH_VM_PID_OFFSET * grinch_id;

	return 0;
}
