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

#ifdef ARCH_RISCV
#define ELF_ARCH EM_RISCV
#endif

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

		vma = uvma_create(task, base, page_up(phdr->p_memsz), vma_flags);
		if (IS_ERR(vma))
			return PTR_ERR(vma);

		src = (void *)ehdr + phdr->p_offset;
		copy_to_user(&task->mm, base, src, phdr->p_memsz);
	}

	vma = uvma_create(task, (void *)USER_STACK_BASE, USER_STACK_SIZE,
			  VMA_FLAG_USER | VMA_FLAG_ZERO | VMA_FLAG_RW);
	if (IS_ERR(vma))
		return PTR_ERR(vma);

	task_set_context(task, ehdr->e_entry, USER_STACK_BASE + USER_STACK_SIZE);

	return 0;
}

static void task_destroy(struct task *task)
{
	uvmas_destroy(task);

	if (task->mm.page_table)
		kfree(task->mm.page_table);
}

struct task *task_from_elf(void *elf)
{
	struct task *task;
	int err;

	task = kmalloc(sizeof(*task));
	if (!task)
		return ERR_PTR(-ENOMEM);

	task->mm.page_table = kzalloc(PAGE_SIZE);
	if (!task->mm.page_table) {
		err = -ENOMEM;
		goto destroy_out;
	}

	INIT_LIST_HEAD(&task->mm.vmas);

	memset(&task->regs, 0, sizeof(task->regs));

	err = task_load_elf(task, elf);
	if (err)
		goto destroy_out;

	return task;

destroy_out:
	task_destroy(task);
	kfree(task);
	return ERR_PTR(err);
}
