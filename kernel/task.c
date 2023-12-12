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

#include <grinch/errno.h>
#include <grinch/elf.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/string.h>
#include <grinch/percpu.h>
#include <grinch/alloc.h>
#include <grinch/uaccess.h>
#include <grinch/vfs.h>

#ifdef ARCH_RISCV
#define ELF_ARCH EM_RISCV
#endif

static LIST_HEAD(task_list);
static DEFINE_SPINLOCK(task_lock);
static pid_t next_pid = 1;

static int task_load_elf(struct task *task, Elf64_Ehdr *ehdr)
{
	unsigned int vma_flags;
	Elf64_Phdr *phdr;
	struct vma *vma;
	unsigned int d;
	void *src, *base;

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG))
		return trace_error(-EINVAL);

	if (ehdr->e_machine != ELF_ARCH)
		return -EINVAL;

	// TODO: Check for out of bounds in ehdr

	phdr = (Elf64_Phdr*)((void*)ehdr + ehdr->e_phoff);
	for (d = 0; d < ehdr->e_phnum; d++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_align != PAGE_SIZE)
			return -EINVAL;

		base = (void *)phdr->p_vaddr;

		vma_flags = VMA_FLAG_USER | VMA_FLAG_ZERO;
		if (phdr->p_flags & PF_R)
			vma_flags |= VMA_FLAG_R;
		if (phdr->p_flags & PF_W)
			vma_flags |= VMA_FLAG_W;
		if (phdr->p_flags & PF_X)
			vma_flags |= VMA_FLAG_EXEC;

		vma = uvma_create(&task->process, base, page_up(phdr->p_memsz), vma_flags);
		if (IS_ERR(vma))
			return PTR_ERR(vma);

		src = (void *)ehdr + phdr->p_offset;
		copy_to_user(&task->process.mm, base, src, phdr->p_memsz);
	}

	vma = uvma_create(&task->process, (void *)USER_STACK_BASE, USER_STACK_SIZE,
			  VMA_FLAG_USER | VMA_FLAG_ZERO | VMA_FLAG_RW);
	if (IS_ERR(vma))
		return PTR_ERR(vma);

	task_set_context(task, ehdr->e_entry, USER_STACK_BASE + USER_STACK_SIZE);

	return 0;
}

void task_destroy(struct task *task)
{
	uvmas_destroy(&task->process);

	if (task->process.mm.page_table)
		kfree(task->process.mm.page_table);
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
	int err;

	task = kmalloc(sizeof(*task));
	if (!task)
		return ERR_PTR(-ENOMEM);

	task->process.mm.page_table = kzalloc(PAGE_SIZE);
	if (!task->process.mm.page_table) {
		err = -ENOMEM;
		goto free_out;
	}

	INIT_LIST_HEAD(&task->process.mm.vmas);
	memset(&task->regs, 0, sizeof(task->regs));
	task->pid = get_new_pid();
	task->state = SUSPENDED;

	return task;

free_out:
	kfree(task);
	return ERR_PTR(err);
}

int task_from_fs(struct task *task, const char *pathname)
{
	void *elf;
	int err;

	elf = vfs_read_file(pathname);
	if (IS_ERR(elf))
		return PTR_ERR(elf);

	err = task_load_elf(task, elf);
	kfree(elf);

	return err;
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

	arch_process_activate(&task->process);
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
	new = task_alloc_new();
	if (IS_ERR(new))
		return PTR_ERR(new);

	new->regs = this->regs;
	regs_set_retval(&new->regs, 0);

	list_for_each(pos, &this->process.mm.vmas) {
		vma = list_entry(pos, struct vma, vmas);
		err = uvma_duplicate(&new->process, &this->process, vma);
		if (err)
			goto destroy_out;
	}

	sched_enqueue(new);

	return new->pid;

destroy_out:
	task_destroy(new);
	kfree(new);
	return err;
}

void prepare_user_return(void)
{
	if (!this_per_cpu()->current_task)
		panic("Nothing to schedule!\n");

	if (this_per_cpu()->schedule)
		schedule();
	else if (this_per_cpu()->pt_needs_update) {
		arch_process_activate(&current_task()->process);
		this_per_cpu()->pt_needs_update = false;
	}
	arch_task_restore();
}

int do_execve(const char *pathname, char *const argv[], char *const envp[])
{
	struct task *this;
	char buf[128];

	this = current_task();
	copy_from_user(&this->process.mm, buf, pathname, sizeof(buf));
	buf[sizeof(buf) - 1] = 0;

	uvmas_destroy(&this->process);
	this_per_cpu()->pt_needs_update = true;

	return task_from_fs(this, buf);
}
