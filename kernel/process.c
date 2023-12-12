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

#define dbg_fmt(x)	"process: " x

#include <asm_generic/grinch_layout.h>

#include <grinch/alloc.h>
#include <grinch/elf.h>
#include <grinch/errno.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>
#include <grinch/percpu.h>
#include <grinch/vfs.h>

#ifdef ARCH_RISCV
#define ELF_ARCH EM_RISCV
#endif

static int process_load_elf(struct task *task, Elf64_Ehdr *ehdr)
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

		vma = uvma_create(task->process, base, page_up(phdr->p_memsz), vma_flags);
		if (IS_ERR(vma))
			return PTR_ERR(vma);

		src = (void *)ehdr + phdr->p_offset;
		copy_to_user(&task->process->mm, base, src, phdr->p_memsz);
	}

	vma = uvma_create(task->process, (void *)USER_STACK_BASE, USER_STACK_SIZE,
			  VMA_FLAG_USER | VMA_FLAG_ZERO | VMA_FLAG_RW);
	if (IS_ERR(vma))
		return PTR_ERR(vma);

	task_set_context(task, ehdr->e_entry, USER_STACK_BASE + USER_STACK_SIZE);

	return 0;
}

int process_from_fs(struct task *task, const char *pathname)
{
	void *elf;
	int err;

	elf = vfs_read_file(pathname);
	if (IS_ERR(elf))
		return PTR_ERR(elf);

	err = process_load_elf(task, elf);
	kfree(elf);

	return err;
}

void process_destroy(struct process *process)
{
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
	task->process = kmalloc(sizeof(*task->process));
	if (!task->process)
		goto free_out;

	task->process->mm.page_table = kzalloc(PAGE_SIZE);
	if (!task->process->mm.page_table)
		goto process_free_out;

	INIT_LIST_HEAD(&task->process->mm.vmas);

	return task;

process_free_out:
	kfree(task->process);

free_out:
	kfree(task);
	return ERR_PTR(-ENOMEM);
}

int do_execve(const char *pathname, char *const argv[], char *const envp[])
{
	struct task *this;
	char buf[128];

	this = current_task();
	copy_from_user(&this->process->mm, buf, pathname, sizeof(buf));
	buf[sizeof(buf) - 1] = 0;

	uvmas_destroy(this->process);
	this_per_cpu()->pt_needs_update = true;

	return process_from_fs(this, buf);
}
