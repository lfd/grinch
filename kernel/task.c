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

#include <grinch/alloc.h>
#include <grinch/arch.h>
#include <grinch/boot.h>
#include <grinch/errno.h>
#include <grinch/hypercall.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/percpu.h>
#include <grinch/timer.h>
#include <grinch/uaccess.h>

#define GRINCH_VM_PID_OFFSET	10000

struct task *init_task;

static LIST_HEAD(task_list);
static LIST_HEAD(timer_list);

static DEFINE_SPINLOCK(task_lock);
static pid_t next_pid = 1;

static inline void _sched_dequeue(struct task *task)
{
	list_del(&task->tasks);
}

void sched_dequeue(struct task *task)
{
	spin_lock(&task_lock);
	_sched_dequeue(task);
	spin_unlock(&task_lock);
}

void sched_enqueue(struct task *task)
{
	spin_lock(&task_lock);
	list_add(&task->tasks, &task_list);
	spin_unlock(&task_lock);
}

/* must hold the parent's lock */
static int task_notify_wait(struct task *parent, struct task *child)
{
	int status;

	if (!parent->wait_for.waiting)
		return -ECHILD;

	if (parent->wait_for.pid != -1 && parent->wait_for.pid != child->pid)
		return -ECHILD;

	if (child->state != TASK_EXIT_DEAD)
		BUG();

	/* Forward status code */
	if (parent->wait_for.status) {
		status = (child->exit_code & 0xff) << 8;
		copy_to_user(&parent->process->mm, parent->wait_for.status,
			     &status, sizeof(status));
	}

	/*
	 * Parent's state might also be TASK_RUNNING, if we have a direct
	 * notification
	 */
	regs_set_retval(&parent->regs, child->pid);
	if (parent->state == TASK_WAIT)
		parent->state = TASK_RUNNABLE;

	task_destroy(child);
	parent->wait_for.waiting = false;

	return 0;
}

long task_wait(pid_t pid, int __user *wstatus, int options)
{
	struct task *task, *child;
	int err;

	/* not supported at the moment */
	if (options)
		return -EINVAL;

	/* not supported at the moment */
	if (pid == 0 || pid < -1)
		return -ENOSYS;

	task = current_task();
	spin_lock(&task->lock);

	if (list_empty(&task->children)) {
		err = -ECHILD;
		goto unlock_out;
	}

	if (pid == -1) {
		list_for_each_entry(child, &task->children, sibling) {
			spin_lock(&child->lock);
			if (child->state == TASK_EXIT_DEAD)
				goto found;
			spin_unlock(&child->lock);
		}
		child = NULL;
		goto found;
	} else {
		list_for_each_entry(child, &task->children, sibling) {
			spin_lock(&child->lock);
			if (child->pid == pid)
				goto found;
			spin_unlock(&child->lock);
		}
	}

	spin_unlock(&task->lock);
	return -ECHILD;

found:
	task->wait_for.pid = pid;
	task->wait_for.waiting = true;
	task->wait_for.status = wstatus;

	if (child) {
		if (child->state == TASK_EXIT_DEAD) {
			err = task_notify_wait(task, child);
			if (err)
				BUG();
			goto unlock_out;
		} else
			spin_unlock(&child->lock);
	}

	task->state = TASK_WAIT;
	this_per_cpu()->schedule = true;
	this_per_cpu()->current_task = NULL;
	err = 0;

unlock_out:
	spin_unlock(&task->lock);
	return err;
}

void task_exit(struct task *task, int code)
{
	struct task *parent, *child, *tmp;

	parent = task->parent;
	if (!parent) {
		pr_warn("Exit from init task!\n");
		BUG();
	}

	task_cancel_timer(task);

	/*
	 * The task is no scheduleable entry any longer, so remove it from the
	 * scheduler list
	 */
	if (!list_empty(&task->tasks))
		sched_dequeue(task);

	/* Take the parent's lock first to prevent deadlock situations */
	spin_lock(&parent->lock);
	spin_lock(&task->lock);
	switch (task->type) {
	case GRINCH_PROCESS:
		process_destroy(task);
		break;

	case GRINCH_VMACHINE:
		vmachine_destroy(task);
		break;

	default:
		BUG();
		break;
	}

	task->state = TASK_EXIT_DEAD;
	task->exit_code = code;

	/*
	 * Do reparenting. Check if the task has children. If so, they go to
	 * PID 1
	 */
	if (parent != init_task)
		spin_lock(&init_task->lock);
	list_for_each_entry_safe(child, tmp, &task->children, sibling) {
		spin_lock(&child->lock);

		list_del(&child->sibling);
		child->parent = init_task;
		list_add(&child->sibling, &init_task->children);

		spin_unlock(&child->lock);
	}
	if (parent != init_task)
		spin_unlock(&init_task->lock);

	/*
	 * Once we have proper support for threads, this won't work like that
	 * any longer. We first have to make sure, that a task got suspended
	 * before being dequeued.
	 */

	if (this_per_cpu()->current_task == task) {
		this_per_cpu()->schedule = true;
		this_per_cpu()->current_task = NULL;
	}

	task_notify_wait(parent, task);
	spin_unlock(&parent->lock);
	spin_unlock(&task->lock);
}

/* must hold the parent's lock */
void task_destroy(struct task *task)
{
	if (task->state != TASK_EXIT_DEAD && task->state != TASK_INIT)
		BUG();

	list_del(&task->sibling);

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

	spin_init(&task->lock);
	task->pid = get_new_pid();
	task->state = TASK_INIT;
	task->type = GRINCH_UNDEF;
	INIT_LIST_HEAD(&task->tasks);
	INIT_LIST_HEAD(&task->timer.timer_list);
	INIT_LIST_HEAD(&task->sibling);
	INIT_LIST_HEAD(&task->children);

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
	task->on_cpu = this_cpu_id();

#ifdef DEBUG
	pr_dbg("CPU %lu took PID %d\n", this_cpu_id(), task->pid);
#endif

	switch (task->type) {
	case GRINCH_PROCESS:
		arch_process_activate(task->process);
		break;

	case GRINCH_VMACHINE:
		arch_vmachine_activate(task->vmachine);
		break;

	default:
		BUG();
		break;
	}
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
		if (task->state == TASK_WFE)
			task = NULL;
		goto out;
	}

	task = tpcpu->current_task;
	list_for_each_entry_from(task, &task_list, tasks) {
		spin_lock(&task->lock);
		if (task->state == TASK_RUNNABLE)
			goto out;
		spin_unlock(&task->lock);
	}

begin:
	if (list_empty(&task_list))
		goto nothing;

	list_for_each_entry(task, &task_list, tasks) {
		if (task == tpcpu->current_task)
			break;

		spin_lock(&task->lock);
		if (task->state == TASK_RUNNABLE)
			goto out;
		spin_unlock(&task->lock);
	}

nothing:
	/*
	 * We have nothing to schedule. But is the current task running and may
	 * continue?
	 */
	if (current_task() && current_task()->state == TASK_RUNNING) {
		task = current_task();
		spin_lock(&task->lock);
	} else
		task = NULL;

out:
	/* If task is set, we arrive here with the task->lock hold */
	task_activate(task);
	if (task)
		spin_unlock(&task->lock);

	spin_unlock(&task_lock);
}

int do_fork(void)
{
	struct task *this, *new;
	struct vma *vma;
	int fd, err;

	new = process_alloc_new();
	if (IS_ERR(new))
		return PTR_ERR(new);
	spin_lock(&new->lock);

	this = current_task();
	spin_lock(&this->lock);
	for (fd = 0; fd < MAX_FDS; fd++)
		if (this->process->fds[fd].fp) {
			pr_warn("fork: open file handles not supported\n");
			err = -ENOSYS;
			goto destroy_out;
		}

	new->regs = this->regs;
	new->parent = this;
	regs_set_retval(&new->regs, 0);

	list_for_each_entry(vma, &this->process->mm.vmas, vmas) {
		err = uvma_duplicate(new->process, this->process, vma);
		if (err)
			goto destroy_out;
	}

	new->state = TASK_RUNNABLE;

	list_add(&new->sibling, &this->children);

	spin_unlock(&this->lock);
	spin_unlock(&new->lock);

	sched_enqueue(new);
	sched_all();

	return new->pid;

destroy_out:
	spin_unlock(&new->lock);
	task_exit(new, err);
	task_destroy(new);
	spin_unlock(&this->lock);

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

static bool timer_comp(struct list_head *_a, struct list_head *_b)
{
	struct task *a = list_entry(_a, struct task, timer.timer_list);
	struct task *b = list_entry(_b, struct task, timer.timer_list);

	return a->timer.expiration > b->timer.expiration;
}

void task_sleep_until(struct task *task, unsigned long long wall_ns)
{
	spin_lock(&task_lock);

	/* VMs remain runnable */
	if (task->type == GRINCH_PROCESS)
		_task_set_wfe(task);

	task->timer.expiration = wall_ns;

	/* If the timer is already queued, remove it first */
	if (!list_empty(&task->timer.timer_list))
		list_del(&task->timer.timer_list);

	list_add_sorted(&timer_list, &task->timer.timer_list, timer_comp);
	this_per_cpu()->handle_events = true;

	spin_unlock(&task_lock);
}

void task_cancel_timer(struct task *task)
{
	spin_lock(&task_lock);

	list_del(&task->timer.timer_list);
	INIT_LIST_HEAD(&task->timer.timer_list);

	spin_unlock(&task_lock);
}

void task_handle_events(void)
{
	struct task *task, *tmp, *update;

	spin_lock(&task_lock);

	update = NULL;
	list_for_each_entry_safe(task, tmp, &timer_list, timer.timer_list) {
		/* sanity check */
		if (task->state != TASK_WFE && task->type != GRINCH_VMACHINE)
			BUG();

		if (task->timer.expiration <= timer_get_wall()) {
			if (task->type == GRINCH_VMACHINE) {
				vmachine_set_timer_pending(task->vmachine);
				if (task->state == TASK_RUNNING) {
					if (task->on_cpu != this_cpu_id()) {
						ipi_send(task->on_cpu);
						continue;
					}
				} else {
					task->state = TASK_RUNNABLE;
				}
			} else { /* TASK_PROCESS */
				task->state = TASK_RUNNABLE;
			}
			if (task->state == TASK_WFE)
				BUG();
			list_del(&task->timer.timer_list);
			INIT_LIST_HEAD(&task->timer.timer_list);
			continue;
		}

		update = task;
		break;
	}

	timer_update(update);
	spin_unlock(&task_lock);
}

static void do_idle(void)
{
	this_per_cpu()->idling = true;
	arch_do_idle();
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
	if (tpcpu->handle_events) {
		tpcpu->handle_events = false;
		task_handle_events();
	}

	if (tpcpu->schedule)
		schedule();

	if (!tpcpu->current_task) {
		spin_lock(&task_lock);
		if (list_empty(&task_list)) {
			spin_unlock(&task_lock);
			if (tpcpu->primary) {
				pr("Nothing to schedule!\n");
				arch_shutdown(-ENOENT);
			}
		} else {
			spin_unlock(&task_lock);
			if (tpcpu->idling)
				BUG();
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
				BUG();
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
