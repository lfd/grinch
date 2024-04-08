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
#include <grinch/fs.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>
#include <grinch/percpu.h>
#include <grinch/syscall.h>
#include <grinch/vfs.h>

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

	/* Load process */
	phdr = (Elf64_Phdr*)((void*)ehdr + ehdr->e_phoff);
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
			vma_flags |= VMA_FLAG_EXEC;

		/* The region must not collide with the stack */
		if (base + page_up(phdr->p_memsz) >= (void *)USER_STACK_BOTTOM)
			return -EINVAL;

		vma = uvma_create(task, base, page_up(phdr->p_memsz),
				  vma_flags, NULL);
		if (IS_ERR(vma))
			return PTR_ERR(vma);

		src = (void *)ehdr + phdr->p_offset;
		copied = copy_to_user(task, base, src, phdr->p_memsz);
		if (copied != phdr->p_memsz)
			return -ERANGE;
	}

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

struct task *process_alloc_new(void)
{
	struct task *task;

	task = task_alloc_new();
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

int sys_execve(const char __user *pathname, char *const __user uargv[],
	       char *const __user uenvp[])
{
	struct uenv_array argv, envp;
	char buf[MAX_PATHLEN];
	struct task *this;
	ssize_t ret;
	int err;

	this = current_task();
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

	uvmas_destroy(&this->process);
	this_per_cpu()->pt_needs_update = true;

	err = process_from_fs(this, buf, &argv, &envp);

	uenv_free(&envp);

uargv_free_out:
	uenv_free(&argv);

	return err;
}
