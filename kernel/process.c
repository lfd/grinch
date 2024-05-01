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

#define dbg_fmt(x)	"process: " x

#include <grinch/align.h>
#include <grinch/alloc.h>
#include <grinch/elf.h>
#include <grinch/errno.h>
#include <grinch/fs/vfs.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>
#include <grinch/percpu.h>
#include <grinch/syscall.h>

#ifdef ARCH_RISCV
#define ELF_ARCH EM_RISCV
#endif

#define ARG_MAX		PAGE_SIZE

static void __user *
fill_uenv_table(struct task *t, const struct uenv_array *uenv,
		void __user *stack_top, char __user *ustring)
{
	void __user *uptr;
	int err, i;

	/* Terminating sentinel */
	stack_top -= BYTES_PER_LONG;
	uptr = NULL;
	err = uptr_to_user(t, stack_top, uptr);
	if (err)
		return ERR_PTR(err);

	/* Remaining arguments in reverse order */
	if (uenv) {
		for (i = uenv->elements - 1; i >= 0; i--) {
			stack_top -= BYTES_PER_LONG;
			uptr = ustring + uenv->cuts[i];
			err = uptr_to_user(t, stack_top, uptr);
			if (err)
				return ERR_PTR(err);
		}
	}
	return stack_top;
}

static ssize_t uenv_sz(const struct uenv_array *uenv)
{
	ssize_t ret;

	/* If no environment, we only have a null-terminator */
	if (!uenv)
		return 1 * sizeof(char *);

	/* Length of the tokenised string itself */
	ret = uenv->length;

	/* Length of the array + null terminator */
	ret += (uenv->elements + 1) * sizeof(char *);

	return ret;
}

static int process_load_elf(struct task *task, Elf64_Ehdr *ehdr,
			    const struct uenv_array *argv,
			    const struct uenv_array *envp)
{
	char __user *uargv_string, *uenvp_string;
	void __user *stack_top;
	unsigned long uargs[3];
	unsigned int vma_flags;
	unsigned long copied;
	Elf64_Phdr *phdr;
	void *src, *base;
	struct vma *vma;
	size_t vma_size;
	unsigned int d;

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG))
		return trace_error(-EINVAL);

	if (ehdr->e_machine != ELF_ARCH)
		return -EINVAL;

	// TODO: Check for out of bounds in ehdr

	/* check if arguments exceed ARG_MAX size */
	copied = uenv_sz(argv) + uenv_sz(envp) + sizeof(uargs);
	if (copied > ARG_MAX)
		return -E2BIG;

	/* Prepare user stack */
	stack_top = (void *)USER_STACK_TOP;

	vma_flags = VMA_FLAG_USER | VMA_FLAG_RW | VMA_FLAG_LAZY;
	vma = uvma_create(task, (void *)USER_STACK_BOTTOM, USER_STACK_SIZE,
			  vma_flags, "[stack]");
	if (IS_ERR(vma))
		return PTR_ERR(vma);

	if (envp) {
		stack_top -= envp->length;
		uenvp_string = stack_top;
		copied = copy_to_user(task, stack_top, envp->string,
				      envp->length);
		if (copied != envp->length)
			return -ENOMEM;
	} else
		uenvp_string = NULL;

	if (argv) {
		stack_top -= argv->length;
		uargv_string = stack_top;
		copied = copy_to_user(task, stack_top, argv->string,
				      argv->length);
		if (copied != argv->length)
			return -ENOMEM;
	} else
		uargv_string = NULL;

	stack_top = PTR_ALIGN_DOWN(stack_top, BYTES_PER_LONG);

	stack_top = fill_uenv_table(task, envp, stack_top, uenvp_string);
	if (IS_ERR(stack_top))
		return PTR_ERR(stack_top);
	uargs[2] = (u64)stack_top;

	stack_top = fill_uenv_table(task, argv, stack_top, uargv_string);
	if (IS_ERR(stack_top))
		return PTR_ERR(stack_top);
	uargs[1] = (u64)stack_top;
	uargs[0] = argv ? argv->elements : 0;

	stack_top -= sizeof(uargs);
	copied = copy_to_user(task, stack_top, &uargs, sizeof(uargs));
	if (copied != sizeof(uargs))
		return -ENOMEM;

	/* Load process */
	phdr = (Elf64_Phdr*)((void*)ehdr + ehdr->e_phoff);
	task->process.brk = NULL;
	for (d = 0; d < ehdr->e_phnum; d++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_align != PAGE_SIZE)
			return -EINVAL;

		base = (void *)phdr->p_vaddr;

		vma_flags = VMA_FLAG_USER;
		if (phdr->p_flags & PF_R)
			vma_flags |= VMA_FLAG_R;
		if (phdr->p_flags & PF_W)
			vma_flags |= VMA_FLAG_W;
		if (phdr->p_flags & PF_X)
			vma_flags |= VMA_FLAG_X;

		/* The region must not collide with any other VMA */
		vma_size = page_up(phdr->p_memsz);
		if (uvma_collides(&task->process, base, vma_size))
			return -EINVAL;

		vma = uvma_create(task, base, vma_size, vma_flags, NULL);
		if (IS_ERR(vma))
			return PTR_ERR(vma);

		src = (void *)ehdr + phdr->p_offset;
		copied = copy_to_user(task, base, src, phdr->p_memsz);
		if (copied != phdr->p_memsz)
			return -ERANGE;

		if (base + vma_size > task->process.brk)
			task->process.brk = base + vma_size;
	}

	task_set_context(task, ehdr->e_entry, (uintptr_t)stack_top);

	return 0;
}

int process_from_fs(struct task *task, const char *pathname,
		    struct uenv_array *argv, struct uenv_array *envp)
{
	void *elf;
	int err;

	elf = vfs_read_file(pathname, NULL);
	if (IS_ERR(elf))
		return PTR_ERR(elf);

	err = process_load_elf(task, elf, argv, envp);
	kfree(elf);

	return err;
}

void process_destroy(struct task *task)
{
	struct process *process;
	unsigned int i;

	process = &task->process;
	for (i = 0; i < MAX_FDS; i++)
		if (process->fds[i].fp)
			file_close(&process->fds[i]);

	uvmas_destroy(process);

	if (process->mm.page_table)
		kfree(process->mm.page_table);
}

struct task *process_alloc_new(const char *name)
{
	struct task *task;

	task = task_alloc_new(name);
	if (IS_ERR(task))
		return task;

	task->type = GRINCH_PROCESS;
	task->process.mm.page_table = kzalloc(PAGE_SIZE);
	if (!task->process.mm.page_table) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}

	INIT_LIST_HEAD(&task->process.mm.vmas);

	return task;
}

int process_handle_fault(struct task *task, void __user *addr, bool is_write)
{
	struct vma *vma;
	int err;

	vma = uvma_find(&task->process, addr);
	if (!vma) {
		pr_warn("PID %d: No VMA found %p\n", task->pid, addr);
		return -ENOENT;
	}

	err = uvma_handle_fault(task, vma, addr);
	if (err) {
		pr_warn("PID %d: Unable to handle fault: %pe\n",
			task->pid, ERR_PTR(err));
		return err;
	}

	if (task == current_task())
		this_per_cpu()->pt_needs_update = true;

	return err;
}

long sys_execve(const char __user *pathname, char *const __user uargv[],
		char *const __user uenvp[])
{
	struct uenv_array argv, envp;
	struct process *process;
	char buf[MAX_PATHLEN];
	struct task *this;
	const char *name;
	ssize_t ret;
	int err;

	this = current_task();
	process = &this->process;
	ret = ustrncpy(buf, pathname, sizeof(buf));
	/* pathname too long */
	if (unlikely(ret == sizeof(buf)))
		return -ERANGE;
	else if (unlikely(ret < 0))
		return ret;

	err = uenv_dup(this, uargv, &argv);
	if (err)
		return err;

	err = uenv_dup(this, uenvp, &envp);
	if (err)
		goto uargv_free_out;

	name = argv.elements ? argv.string : "NO NAME";
	task_set_name(this, name);

	uvmas_destroy(process);
	process->brk = NULL;
	process->vma_heap = NULL;
	this_per_cpu()->pt_needs_update = true;

	err = process_from_fs(this, buf, &argv, &envp);

	uenv_free(&envp);

uargv_free_out:
	uenv_free(&argv);

	return err;
}

long sys_brk(unsigned long addr)
{
	struct process *process;
	unsigned int vma_flags;
	struct vma *vma_heap;
	struct task *task;
	unsigned long brk;
	size_t size;

	task = current_task();
	process = &task->process;

	spin_lock(&task->lock);
	brk = (unsigned long)process->brk;
	if (!addr)
		goto unlock_out;

	/* can not shift break to the left */
	if (addr < brk) {
		brk = -EINVAL;
		goto unlock_out;
	}

	addr = page_up(addr);
	size = addr - brk;

	/* We don't support shrinking or extending the heap at the moment */
	if (process->vma_heap) {
		brk = -ENOSYS;
		goto unlock_out;
	}

	vma_flags = VMA_FLAG_USER | VMA_FLAG_RW | VMA_FLAG_LAZY;
	vma_heap = uvma_create(task, (void *)brk, size, vma_flags, "[heap]");
	if (IS_ERR(vma_heap)) {
		brk = PTR_ERR(vma_heap);
		goto unlock_out;
	}

	process->vma_heap = vma_heap;
	brk = brk + size;

unlock_out:
	spin_unlock(&task->lock);
	return brk;
}
