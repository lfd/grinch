/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"task: " x

#include <grinch/alloc.h>
#include <grinch/arch.h>
#include <grinch/atomic.h>
#include <grinch/boot.h>
#include <grinch/cpu.h>
#include <grinch/errno.h>
#include <grinch/gconfig.h>
#include <grinch/gfp.h>
#include <grinch/grinch_guest.h>
#include <grinch/panic.h>
#include <grinch/reboot.h>
#include <grinch/string.h>
#include <grinch/syscall.h>
#include <grinch/task.h>
#include <grinch/vsprintf.h>

#define GRINCH_VM_PID_OFFSET	10000

/*
 * Written to the base of every kernel stack; no guard page, so this word is
 * all that tells an overflow happened. Linux's STACK_END_MAGIC.
 */
#define STACK_END_MAGIC	0x57AC6E9D

#define KERNEL_STACK_SIZE	(CONFIG_KERNEL_STACK_PAGES * PAGE_SIZE)

static inline unsigned long *end_of_stack(struct task *task)
{
	return task->stack;
}

struct task *init_task;

static LIST_HEAD(task_list);
static LIST_HEAD(timer_list);

static DEFINE_SPINLOCK(task_lock);

static atomic_t next_pid = ATOMIC_INIT(1);

static void task_restore(void);
static void schedule(void);
static void __task_wake(struct task *task);

static void task_dequeue(struct task *task)
{
	spin_lock(&task_lock);
	list_del(&task->tasks);
	spin_unlock(&task_lock);
}

void task_enqueue(struct task *task)
{
	spin_lock(&task_lock);
	list_add(&task->tasks, &task_list);
	spin_unlock(&task_lock);
}

static inline void _task_set_wfe(struct task *task)
{
	if (task->state == TASK_WFE)
		BUG();

	/* Ensure that the WFE type is really set */
	if (task->wfe == WFE_NONE)
		BUG();

	task->state = TASK_WFE;
}

void task_set_wfe(struct task *task)
{
	spin_lock(&task_lock);
	_task_set_wfe(task);
	spin_unlock(&task_lock);
}

/* Drop this CPU's ownership claim; publishes our prior writes to the task. */
static void task_release_cpu(struct task *task)
{
	smp_store_release(&task->on_cpu, TASK_NO_CPU);
}

SYSCALL_DEF3(wait, pid_t, pid, int __user *, wstatus, int, options)
{
	struct task *task, *child;
	pid_t child_pid;
	int status;
	bool seen;

	/* not supported at the moment */
	if (options)
		return -EINVAL;

	/* not supported at the moment */
	if (pid == 0 || pid < -1)
		return -ENOSYS;

	task = current_task();

	for (;;) {
		spin_lock(&task->lock);

		if (list_empty(&task->children)) {
			spin_unlock(&task->lock);
			return -ECHILD;
		}

		seen = false;
		list_for_each_entry(child, &task->children, sibling) {
			if (pid != -1 && child->pid != pid)
				continue;

			seen = true;
			spin_lock(&child->lock);
			if (child->state == TASK_EXIT_DEAD)
				goto reap;
			spin_unlock(&child->lock);
		}

		if (pid != -1 && !seen) {
			spin_unlock(&task->lock);
			return -ECHILD;
		}

		/*
		 * Mark under our own lock: task_exit() wakes the parent under
		 * the same lock, so no death between the scan and the sleep
		 * is lost. A death of the wrong child just rescans.
		 */
		task->wfe = WFE_CHILD;
		_task_set_wfe(task);
		spin_unlock(&task->lock);

		task_sleep();
	}

reap:
	if (wstatus) {
		status = (child->exit_code & 0xff) << 8;
		copy_to_user(task, wstatus, &status, sizeof(status));
	}

	child_pid = child->pid;
	spin_unlock(&child->lock);
	task_put(child);
	spin_unlock(&task->lock);

	return child_pid;
}

void task_exit(struct task *task, int code)
{
	struct task *parent, *child, *tmp;

	/* The dying task's stack may be freed here: never our own. */
	if (this_per_cpu()->current_task == task)
		BUG();

	parent = task->parent;
	if (!parent) {
		pr_warn("init exited with code %d\n", code);
		BUG();
	}

	task_cancel_timer(task);

	/*
	 * The task is no scheduleable entry any longer, so remove it from the
	 * scheduler list
	 */
	if (!list_empty(&task->tasks))
		task_dequeue(task);

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

	/* The parent reaps on its own stack; all it needs is a wake. */
	if (parent->wfe == WFE_CHILD)
		__task_wake(parent);

	spin_unlock(&parent->lock);
	spin_unlock(&task->lock);
}

/*
 * A task taking itself down must not become reapable while it still runs on
 * its own stack: mark it dying and let whoever follows it on this CPU finish
 * the job from safe ground (task_switch_finish()).
 */
void task_exit_self(int code)
{
	struct task *task = current_task();

	task_cancel_timer(task);

	spin_lock(&task->lock);
	task->exit_code = code;
	task->state = TASK_EXIT_DYING;
	spin_unlock(&task->lock);

	this_per_cpu()->schedule = true;
}

/*
 * The caller holds the parent's lock where the task hangs among its children;
 * a task that was never published there is put down without it.
 */
void task_put(struct task *task)
{
	if (task->state != TASK_EXIT_DEAD && task->state != TASK_INIT)
		BUG();

	list_del(&task->sibling);

	free_pages(task->stack, CONFIG_KERNEL_STACK_PAGES);
	kfree(task);
}

static inline pid_t get_new_pid(void)
{
	return atomic_fetch_add_relaxed(1, &next_pid);
}

void task_set_name(struct task *task, const char *name)
{
	strncpy(task->name, name, TASK_NAME_LEN - 1);
}

static struct task *task_alloc(const char *name)
{
	struct task *task;

	task = kzalloc(sizeof(*task));
	if (!task)
		return ERR_PTR(-ENOMEM);

	task_set_name(task, name);

	spin_init(&task->lock);
	task->state = TASK_INIT;
	task->on_cpu = TASK_NO_CPU;
	task->type = GRINCH_UNDEF;
	INIT_LIST_HEAD(&task->tasks);
	INIT_LIST_HEAD(&task->timer_list);
	INIT_LIST_HEAD(&task->sibling);
	INIT_LIST_HEAD(&task->children);

	return task;
}

struct task *task_alloc_new(const char *name)
{
	struct task *task;

	task = task_alloc(name);
	if (IS_ERR(task))
		return task;

	task->stack = alloc_pages(CONFIG_KERNEL_STACK_PAGES);
	if (!task->stack) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}
	*end_of_stack(task) = STACK_END_MAGIC;

	/*
	 * The frame sits at the top of the stack. A task starts from a zeroed
	 * frame: nothing leaks into it, and the origin marker reads "a context
	 * of its own" without anybody setting it.
	 */
	task->regs = task->stack + KERNEL_STACK_SIZE - sizeof(struct registers);
	memset(task->regs, 0, sizeof(*task->regs));

	/* The first switch to this task lands in ret_from_fork. */
	task_setup_thread(task);

	task->pid = get_new_pid();

	return task;
}

/*
 * Give the context this CPU boots on a struct task and a name. The idle task
 * owns no address space and no PID, never enters the task list, always counts
 * as running and never leaves its CPU.
 */
void idle_task_install(void)
{
	struct per_cpu *tpcpu;
	struct task *idle;

	idle = task_alloc("idle");
	if (IS_ERR(idle))
		panic("No memory for the idle task\n");

	idle->type = GRINCH_KTHREAD;
	idle->state = TASK_RUNNING;
	idle->on_cpu = this_cpu_id();

	tpcpu = this_per_cpu();

	/*
	 * The idle task runs on the stack the CPU booted on, and never leaves
	 * the kernel: there is no register frame to point at.
	 */
	idle->stack = tpcpu->stack;
	*end_of_stack(idle) = STACK_END_MAGIC;

	tpcpu->idle_task = idle;
	tpcpu->current_task = idle;
}

static void task_activate(struct task *task)
{
	struct per_cpu *tpcpu;
	struct task *old;

	old = current_task();
	if (old == task) {
		/* sanity check - don't remove */
		if (task->state == TASK_RUNNING)
			return;

		/*
		 * A task that was not scheduled might have changed from
		 * RUNNNING to RUNNABLE during, e.g. during a sleep
		 */
		if (task->state == TASK_RUNNABLE) {
			task->state = TASK_RUNNING;
			return;
		}

		BUG();
	}

	tpcpu = this_per_cpu();
	/*
	 * Only set the task to runnable, if it was running before.
	 * Through a syscall, it might be set to wait for events.
	 * The CPU release follows in task_switch_finish(), once old has
	 * left its stack.
	 */
	if (old && !task_is_idle(old) &&
	    old->state == TASK_RUNNING && old->on_cpu == this_cpu_id())
		old->state = TASK_RUNNABLE;

	tpcpu->current_task = task;
	if (task_is_idle(task)) {
		/* Don't leave the translation root on a process' page table */
		arch_process_deactivate();
		return;
	}

	if (task->state != TASK_RUNNABLE)
		panic("Activating non-runnable task: PID %u state: %x\n",
		      task->pid, task->state);
	task->state = TASK_RUNNING;
	/* Claim ownership; pairs with task_release_cpu() and the pick loop. */
	WRITE_ONCE(task->on_cpu, this_cpu_id());

#ifdef CONFIG_DEBUG_OUTPUT
	pr_dbg("CPU %lu took PID %d\n", this_cpu_id(), task->pid);
#endif

	switch (task->type) {
	case GRINCH_PROCESS:
		arch_process_activate(&task->process);
		break;

	case GRINCH_VMACHINE:
		/* Don't leave the translation root on a process' page table */
		arch_process_deactivate();
		arch_vmachine_activate(&task->vmachine);
		break;

	default:
		BUG();
		break;
	}
}

/*
 * The second half of a context switch, on the incoming task's stack, with
 * task_lock still held from the frozen schedule() frame. Only now has prev
 * fully left its stack, so only now may another CPU pick it up -- or, for a
 * dying task, its memory be given back.
 */
static void task_switch_finish(struct task *prev)
{
	bool dying = prev->state == TASK_EXIT_DYING;

	if (!task_is_idle(prev))
		task_release_cpu(prev);
	spin_unlock(&task_lock);

	if (dying)
		task_exit(prev, prev->exit_code);
}

/* First landing of a task that never ran, called from ret_from_fork. */
void task_entry_finish(struct task *prev)
{
	task_switch_finish(prev);
	task_restore();
}

/*
 * Give up the CPU until task_wake(). The caller has marked the task waiting
 * (task_set_wfe()) before dropping the lock of whoever will wake it, so a
 * wake-up between the mark and this call is not lost: schedule() re-examines
 * the state under task_lock and simply keeps running a task already made
 * runnable again. Returns with the task running, possibly on another CPU.
 */
void task_sleep(void)
{
	schedule();
}

/*
 * Make a waiting task runnable again; the woken task rescans in place.
 * The bare variant is for callers already serialised against the sleeper's
 * mark by another lock -- schedule()'s pick loop takes the task's own lock,
 * so a state write under it cannot tear a decision.
 */
static void __task_wake(struct task *task)
{
	if (task->state == TASK_WFE) {
		task->wfe = WFE_NONE;
		task->state = TASK_RUNNABLE;
	}

	this_per_cpu()->schedule = true;
}

void task_wake(struct task *task)
{
	spin_lock(&task_lock);
	__task_wake(task);
	spin_unlock(&task_lock);
}

/*
 * A task may be activated on this CPU only if no other CPU still owns it.
 * A task we already own (or that is free) stays eligible; one still held by
 * another CPU is skipped until that CPU releases it via task_release_cpu().
 */
static bool task_claimable(struct task *task)
{
	unsigned long owner = READ_ONCE(task->on_cpu);

	return owner == TASK_NO_CPU || owner == this_cpu_id();
}

static void schedule(void)
{
	struct task *task, *old;
	struct per_cpu *tpcpu;

	tpcpu = this_per_cpu();

	if (*end_of_stack(tpcpu->current_task) != STACK_END_MAGIC)
		panic("PID %d: kernel stack overflow\n",
		      tpcpu->current_task->pid);

	spin_lock(&task_lock);
	tpcpu->schedule = false;
	task = NULL;

#if 0
	// Use this chance to allow other VMs to run
	if (grinch_is_guest)
		hypercall_yield();
#endif

	if (task_is_idle(tpcpu->current_task))
		goto begin;

	if (list_is_singular(&task_list)) {
		task = list_first_entry(&task_list, struct task, tasks);
		spin_lock(&task->lock);
		if (task->state == TASK_WFE || !task_claimable(task)) {
			spin_unlock(&task->lock);
			goto nothing;
		}
		goto out;
	}

	task = tpcpu->current_task;
	list_for_each_entry_from(task, &task_list, tasks) {
		spin_lock(&task->lock);
		if (task->state == TASK_RUNNABLE && task_claimable(task))
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
		if (task->state == TASK_RUNNABLE && task_claimable(task))
			goto out;
		spin_unlock(&task->lock);
	}

nothing:
	/*
	 * Nothing else to schedule. The current task continues if it still may
	 * run; otherwise the CPU falls back to its idle task.
	 */
	task = tpcpu->current_task;
	if (task->state != TASK_RUNNING)
		task = tpcpu->idle_task;
	spin_lock(&task->lock);

out:
	/* We arrive here with the task->lock held */
	old = tpcpu->current_task;
	task_activate(task);
	spin_unlock(&task->lock);

	/*
	 * The switch returns here only once this context is scheduled again,
	 * handing back whatever ran last on this CPU. task_switch_finish()
	 * releases task_lock for the frame it resumes.
	 */
	if (old != task)
		task_switch_finish(__switch_to(old, task));
	else
		spin_unlock(&task_lock);
}

SYSCALL_DEF0(fork)
{
	struct task *this, *new;
	struct vma *vma;
	int err;

	/* A process can only be copied where addresses are translated. */
	if (!IS_ENABLED(CONFIG_MMU))
		return -ENOSYS;

	this = current_task();
	new = process_alloc_new(this->name);
	if (IS_ERR(new))
		return PTR_ERR(new);

	*new->regs = *this->regs;
	new->parent = this;
	regs_set_retval(new->regs, 0);

	spin_lock(&this->lock);
	process_dup_fds(this, new);
	spin_unlock(&this->lock);

	/*
	 * The copy runs unlocked: until the new task is enqueued no one else
	 * can reach it, and what it copies cannot change while its parent
	 * stands here.
	 */
	err = process_setcwd(new, this->process.cwd.pathname);
	if (err)
		goto destroy_out;

	list_for_each_entry(vma, &this->process.mm.vmas, vmas) {
		err = uvma_duplicate(new, this, vma);
		if (err)
			goto destroy_out;
	}

	spin_lock(&this->lock);
	spin_lock(&new->lock);
	new->state = TASK_RUNNABLE;
	list_add(&new->sibling, &this->children);
	spin_unlock(&new->lock);
	spin_unlock(&this->lock);

	task_enqueue(new);
	sched_all();

	return new->pid;

destroy_out:
	task_exit(new, err);
	task_put(new);

	return err;
}

static bool timer_comp(struct list_head *_a, struct list_head *_b)
{
	struct task *a = list_entry(_a, struct task, timer_list);
	struct task *b = list_entry(_b, struct task, timer_list);

	return a->deadline > b->deadline;
}

void task_sleep_until(struct task *task, struct timespec *ts)
{
	time_t wall_ns;

	spin_lock(&task_lock);

	if (task->wfe != WFE_NONE) {
		/* A VM may re-arm its timer while a previous one is still pending */
		if (task->type != GRINCH_VMACHINE || task->wfe != WFE_TIMER)
			BUG();
	}

	wall_ns = ts_to_ns(ts);
	task->deadline = wall_ns;
	task->wfe = WFE_TIMER;

	/* VMs remain runnable */
	if (task->type == GRINCH_PROCESS)
		_task_set_wfe(task);

	/* If the timer is already queued, remove it first */
	if (!list_empty(&task->timer_list))
		list_del(&task->timer_list);

	list_add_sorted(&timer_list, &task->timer_list, timer_comp);
	this_per_cpu()->handle_events = true;

	spin_unlock(&task_lock);
}

void task_sleep_for(struct task *task, struct timespec *ts)
{
	struct timespec now, until;

	timer_get_wall(&now);
	until = timespec_add(now, *ts);
	task_sleep_until(task, &until);
}

void task_cancel_timer(struct task *task)
{
	spin_lock(&task_lock);

	list_del(&task->timer_list);
	INIT_LIST_HEAD(&task->timer_list);
	if (task->wfe == WFE_TIMER)
		task->wfe = WFE_NONE;

	spin_unlock(&task_lock);
}

void task_handle_events(void)
{
	struct task *task, *tmp, *update;

	spin_lock(&task_lock);

	update = NULL;
	list_for_each_entry_safe(task, tmp, &timer_list, timer_list) {
		/* sanity check */
		if (task->state != TASK_WFE && task->type != GRINCH_VMACHINE)
			BUG();

		if (task->wfe != WFE_TIMER)
			BUG();

		if (task->deadline <= timer_get_wall_ns()) {
			if (task->type == GRINCH_VMACHINE) {
				vmachine_set_timer_pending(&task->vmachine);
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
			list_del(&task->timer_list);
			INIT_LIST_HEAD(&task->timer_list);
			task->wfe = WFE_NONE;
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
	struct per_cpu *tpcpu;
	bool empty;

	tpcpu = this_per_cpu();

	spin_lock(&task_lock);
	empty = list_empty(&task_list);
	spin_unlock(&task_lock);

	/* An empty task list can never fill up again. */
	if (empty && tpcpu->primary) {
		pr("Nothing to schedule!\n");
		shutdown(-ENOENT);
	}

	/*
	 * Enable interrupts across the idle instruction so a pending IRQ is
	 * taken as a trap and serviced by the arch IRQ handler (which
	 * tolerates the idle context), instead of being polled here. The
	 * handler records a reschedule; we act on it once we unwind.
	 */
	irq_enable();
	wait_for_interrupt();
	irq_disable();

	check_panic();
}

/*
 * The register frame lives where the trap entry put it -- on the task's own
 * stack -- so there is nothing to copy any more. What remains is the virtual
 * machine context, which lives in CSRs the kernel clobbers between exit and
 * re-entry.
 */
static void task_restore(void)
{
	struct task *task = current_task();

	if (task->type == GRINCH_VMACHINE)
		arch_vmachine_restore(&task->vmachine);
}

void task_save(struct registers *regs)
{
	struct task *task;

	task = current_task();
	if (task->type == GRINCH_VMACHINE)
		arch_vmachine_save(&task->vmachine);
}

static const char *task_type_to_string(enum task_type type)
{
	switch (type) {
		case GRINCH_UNDEF:
			return "undef  ";
		case GRINCH_PROCESS:
			return "process";
		case GRINCH_VMACHINE:
			return "VM     ";
		case GRINCH_KTHREAD:
			return "kthread";
		default:
			return "unknown";
	};
}

static const char *task_state_to_string(enum task_state state)
{
	switch (state) {
		case TASK_INIT:
			return "init    ";
		case TASK_RUNNABLE:
			return "runnable";
		case TASK_RUNNING:
			return "running ";
		case TASK_WFE:
			return "wfe     ";
		case TASK_EXIT_DYING:
			return "dying   ";
		case TASK_EXIT_DEAD:
			return "ext dead";
		default:
			return "unkn    ";
	}
}

static const char *task_wfe_to_string(enum task_wfe wfe)
{
	switch (wfe) {
		case WFE_NONE:
			return "none  ";
		case WFE_CHILD:
			return "child ";
		case WFE_TIMER:
			return "timer ";
		case WFE_READ:
			return "read  ";
		default:
			return "unkn  ";
	}
}

#if 0
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

void tasks_dump(void)
{
	const char *on_cpu;
	struct task *task;
	char buf[16];

	spin_lock(&task_lock);

	list_for_each_entry(task, &task_list, tasks) {
		if (task->on_cpu == TASK_NO_CPU) {
			on_cpu = "-";
		} else {
			snprintf(buf, sizeof(buf), "%lu", task->on_cpu);
			on_cpu = buf;
		}
		pr("PID: %u Type: %s State: %s WFE: %s On CPU: %s - %s\n",
		   task->pid, task_type_to_string(task->type),
		   task_state_to_string(task->state),
		   task_wfe_to_string(task->wfe),
		   on_cpu, task->name);
	}

	spin_unlock(&task_lock);
}

void prepare_user_return(void)
{
	struct per_cpu *tpcpu;
	struct task *t;

retry:
	/*
	 * Re-fetched on every lap: schedule() switches stacks, and a context
	 * that froze on one CPU may thaw on another -- nothing per-CPU
	 * survives the call.
	 */
	tpcpu = this_per_cpu();
	if (tpcpu->handle_events) {
		tpcpu->handle_events = false;
		task_handle_events();
	}

	if (tpcpu->schedule)
		schedule();

	if (task_is_idle(current_task())) {
		do_idle();
		goto retry;
	}

	task_restore();

	t = current_task();
	if (t->state != TASK_RUNNING)
		BUG();
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
		atomic_fetch_add_relaxed(GRINCH_VM_PID_OFFSET * grinch_id,
					 &next_pid);

	return 0;
}

void task_handle_fault(void __user *addr, bool is_write)
{
	struct task *task;
	int err;

	task = current_task();
	spin_lock(&task->lock);
	/* not implemented yet */
	if (task->type == GRINCH_VMACHINE)
		BUG();
	if (task->type == GRINCH_PROCESS)
		err = process_handle_fault(task, addr, is_write);
	else
		BUG();
	spin_unlock(&task->lock);

	if (err) {
		/* We have a page fault. */
		pr("PID %d: SEGFAULT at 0x%p (%s)\n", task->pid, addr,
		   is_write ? "write" : "read");
		task_exit_self(-EFAULT);
		return;
	}
}

static struct task *task_by_pid(pid_t pid)
{
	struct task *task;

	list_for_each_entry(task, &task_list, tasks)
		if (task->pid == pid)
			return task;

	return NULL;
}

void process_show_vmas(pid_t pid)
{
	struct process *process;
	struct task *task;
	struct vma *vma;

	spin_lock(&task_lock);
	task = task_by_pid(pid);
	if (!task) {
		pr("PID %d: no task\n", pid);
		spin_unlock(&task_lock);
		return;
	}
	spin_lock(&task->lock);
	spin_unlock(&task_lock);

	process = &task->process;
	pr("VMA map of PID %d (%s)\n", task->pid, task->name);
	list_for_each_entry(vma, &process->mm.vmas, vmas) {
		pr("%p-%p %c%c%c%c %012lx %s\n",
		   vma->base, vma->base + vma->size,
		   (vma->flags & VMA_FLAG_R) ? 'r' : '-',
		   (vma->flags & VMA_FLAG_W) ? 'w' : '-',
		   (vma->flags & VMA_FLAG_X) ? 'x' : '-',
		   (vma->flags & VMA_FLAG_LAZY) ? 'z' : '-',
		   vma->size, vma->name);
	}

	spin_unlock(&task->lock);
}
