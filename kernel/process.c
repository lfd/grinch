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
#include <grinch/device.h>
#include <grinch/elf.h>
#include <grinch/errno.h>
#include <grinch/fs/util.h>
#include <grinch/fs/vfs.h>
#include <grinch/gcall.h>
#include <grinch/pci.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>
#include <grinch/percpu.h>
#include <grinch/syscall.h>
#include <grinch/ttp.h>

#ifdef ARCH_RISCV
#define ELF_ARCH	EM_RISCV
#if ARCH_RISCV == 64 /* rv64 */
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Phdr Elf_Phdr;
#elif ARCH_RISCV == 32 /* rv32 */
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Phdr Elf_Phdr;
#endif /* rv32 */
#endif /* riscv */

#define ARG_MAX		PAGE_SIZE

static void __user *
fill_uenv_table(struct task *t, const struct uenv_array *uenv,
		void __user *base, char __user *ustring)
{
	void __user *uptr;
	unsigned int i;
	int err;

	if (uenv) {
		for (i = 0; i < uenv->elements; i++) {
			uptr = ustring + uenv->cuts[i];
			err = uptr_to_user(t, base, uptr);
			if (err)
				return ERR_PTR(err);
			base += BYTES_PER_LONG;
		}
	}

	/* Terminating sentinel */
	uptr = NULL;
	err = uptr_to_user(t, base, uptr);
	if (err)
		return ERR_PTR(err);
	base += BYTES_PER_LONG;

	return base;
}

static size_t __uenv_elems(const struct uenv_array *uenv)
{
	return uenv->elements + 1;
}

static size_t uenv_elems(const struct uenv_array *uenv)
{
	/*
	 * In case of no environment, we have at least one element: The
	 * sentinel Null pointer
	 */
	if (!uenv)
		return 1;

	return __uenv_elems(uenv);
}

static size_t uenv_sz(const struct uenv_array *uenv)
{
	size_t ret;

	/* If no environment, we only have a null-terminator */
	if (!uenv)
		return 1 * sizeof(char *);

	/* Length of the tokenised string itself */
	ret = uenv->length;

	/* Length of the array + null terminator */
	ret += __uenv_elems(uenv) * sizeof(char *);

	return ret;
}

static int process_load_elf(struct task *task, Elf_Ehdr *ehdr,
			    const struct uenv_array *argv,
			    const struct uenv_array *envp)
{
	char __user *uargv_string, *uenvp_string;
	void __user *stack_top, __user *tmp;
	unsigned int vma_flags;
	unsigned long copied;
	unsigned long argc;
	Elf_Phdr *phdr;
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
	copied = uenv_sz(argv) + uenv_sz(envp) + sizeof(argc);
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

	/*
	 * All strings have just been copied onto the stack. Now he have to
	 * reserve enough memory to place the envp as well as argv table on the
	 * stack. And we need one long for argc.
	 *
	 * The resulting address needs to be aligned down to have the stack
	 * pointer initially on a 64-bit boundary.
	 */
	stack_top -= (uenv_elems(envp) + uenv_elems(argv) + 1) * BYTES_PER_LONG;
	stack_top = PTR_ALIGN_DOWN(stack_top, 8);

	tmp = stack_top;
	/* store argc */
	argc = argv ? argv->elements : 0;
	copied = copy_to_user(task, tmp, &argc, sizeof(argc));
	if (copied != sizeof(argc))
		return -ENOMEM;
	tmp += sizeof(argc);

	tmp = fill_uenv_table(task, argv, tmp, uargv_string);
	if (IS_ERR(stack_top))
		return PTR_ERR(tmp);

	tmp = fill_uenv_table(task, envp, tmp , uenvp_string);
	if (IS_ERR(stack_top))
		return PTR_ERR(tmp);

	/* Load process */
	phdr = (Elf_Phdr*)((void*)ehdr + ehdr->e_phoff);
	task->process.brk.base = NULL;
	for (d = 0; d < ehdr->e_phnum; d++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_align != PAGE_SIZE)
			return -EINVAL;

		base = (void *)(uintptr_t)phdr->p_vaddr;

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
		copied = copy_to_user(task, base, src, phdr->p_filesz);
		if (copied != phdr->p_filesz)
			return -ERANGE;

		if (base + vma_size > task->process.brk.base)
			task->process.brk.base = base + vma_size;
	}

	task_set_context(task, ehdr->e_entry, (uintptr_t)stack_top);

	return 0;
}

int process_from_path(struct task *task, struct file *at, const char *pathname,
		      struct uenv_array *argv, struct uenv_array *envp)
{
	struct file *file;
	void *elf;
	int err;

	file = file_open_at(at, pathname);
	if (IS_ERR(file))
		return PTR_ERR(file);

	elf = vfs_read_file(file, NULL);
	file_close(file);
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

	if (task->type != GRINCH_PROCESS)
		BUG();

	process = &task->process;
	for (i = 0; i < MAX_FDS; i++)
		if (process->fds[i].fp)
			file_close(process->fds[i].fp);

	uvmas_destroy(process);

	if (process->cwd.pathname) {
		file_close(process->cwd.file);
		process->cwd.file = NULL;

		kfree(process->cwd.pathname);
		process->cwd.pathname = NULL;
	}

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

	vma = uvma_at(&task->process, addr);
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

static long _sys_execve(const char __user *_pathname,
			const char *const __user *uargv,
			const char *const __user *uenvp)
{
	struct uenv_array argv, envp;
	struct process *process;
	struct task *this;
	const char *name;
	char *pathname;
	int err;

	this = current_task();
	process = &this->process;

	pathname = pathname_from_user(_pathname, NULL);
	if (IS_ERR(pathname))
		return PTR_ERR(pathname);

	err = uenv_dup(this, uargv, &argv);
	if (err)
		goto pathname_out;

	err = uenv_dup(this, uenvp, &envp);
	if (err)
		goto uargv_free_out;

	name = argv.elements ? argv.string : "NO NAME";
	task_set_name(this, name);

	uvmas_destroy(process);
	process->brk.base = NULL;
	process->brk.vma = NULL;
	this_per_cpu()->pt_needs_update = true;

	err = process_from_path(this, cwd(), pathname, &argv, &envp);

	uenv_free(&envp);

uargv_free_out:
	uenv_free(&argv);

pathname_out:
	kfree(pathname);

	return err;
}

SYSCALL_DEF3(execve, const char __user *, pathname,
	       const char __user *const __user *, uargv,
	       const char __user *const __user *, uenvp)
{
	long ret;
	struct task *cur;

	ret = _sys_execve(pathname, uargv, uenvp);
	if (ret) {
		cur = current_task();
		pr("execve failed on task %u: %pe\n", cur->pid, ERR_PTR(ret));
		task_exit(cur, ret);
	}

	return ret;
}

SYSCALL_DEF1(brk, unsigned long, addr)
{
	struct process *process;
	unsigned long base, brk;
	unsigned int vma_flags;
	struct vma *vma_heap;
	struct task *task;
	size_t size;

	task = current_task();
	process = &task->process;

	base = (unsigned long)process->brk.base;

	spin_lock(&task->lock);
	if (!addr)
		goto report_out;

	/* We only support page-wise changes of the program break */
	if (addr % PAGE_SIZE)
		return -EINVAL;

	/* We can not shift break to the left */
	if (addr < base) {
		brk = -EINVAL;
		goto unlock_out;
	}
	size = addr - base;

	/* Zero-size VMAs are not allowed */
	if (size == 0) {
		brk = -EINVAL;
		goto unlock_out;
	}

	if (!process->brk.vma) {
		vma_flags = VMA_FLAG_USER | VMA_FLAG_RW | VMA_FLAG_LAZY;
		vma_heap = uvma_create(task, process->brk.base, size, vma_flags, "[heap]");
		if (IS_ERR(vma_heap)) {
			brk = PTR_ERR(vma_heap);
			goto unlock_out;
		}
		process->brk.vma = vma_heap;
	} else {
		brk = uvma_resize(process, process->brk.vma, size);
		if (brk)
			goto unlock_out;
	}

report_out:
	brk = base + (process->brk.vma ? process->brk.vma->size : 0);

unlock_out:
	spin_unlock(&task->lock);
	return brk;
}

SYSCALL_DEF1(grinch_usleep, useconds_t, us)
{
	struct timespec ts;

	us_to_ts(us, &ts);
	task_sleep_for(current_task(), &ts);
	this_per_cpu()->schedule = true;

	return 0;
}

SYSCALL_DEF1(exit, long, errno)
{
	task_exit(current_task(), errno);

	return 0;
}

SYSCALL_DEF0(sched_yield)
{
	this_per_cpu()->schedule = true;

	return 0;
}

SYSCALL_DEF0(getpid)
{
	return current_task()->pid;
}

SYSCALL_DEF2(clock_gettime, clockid_t, id, struct timespec __user *, _ts)
{
	struct timespec ts;

	if (id != 0)
		return -EINVAL;

	timer_get_wall(&ts);
	if (copy_to_user(current_task(), _ts, &ts, sizeof(ts)) != sizeof(ts))
		return -EFAULT;

	return 0;
}

SYSCALL_DEF2(grinch_call, unsigned long, no, unsigned long, arg)
{
	long ret;

	ret = 0;
	switch (no) {
		case GCALL_PS:
			tasks_dump();
			break;

		case GCALL_KHEAP:
			kheap_stats();
			break;

		case GCALL_LSPCI:
			pci_lspci();
			break;

		case GCALL_LSOF:
			vfs_lsof();
			break;

		case GCALL_MAPS:
			process_show_vmas(arg);
			break;

		case GCALL_LSDEV:
			dev_list();
			break;

		case GCALL_LOGLEVEL:
			loglevel_set(arg);
			break;

		case GCALL_TTP:
			ret = gcall_ttp(arg);
			break;

		default:
			ret = -ENOSYS;
			break;
	}

	return ret;
}

int process_setcwd(struct task *t, const char *pathname)
{
	struct file *f_new;
	struct process *p;
	char *new;
	int err;

	p = &t->process;

	f_new = file_open_at(p->cwd.file, pathname);
	if (IS_ERR(f_new))
		return PTR_ERR(f_new);

	if (!S_ISDIR(f_new->mode)) {
		err = -ENOTDIR;
		goto close_out;
	}

	new = file_realpath(f_new);
	if (!new) {
		err = -ENOMEM;
		goto close_out;
	}

	if (p->cwd.pathname) {
		file_close(p->cwd.file);
		kfree(p->cwd.pathname);
	}

	p->cwd.pathname = new;
	p->cwd.file = f_new;

	return 0;

close_out:
	file_close(f_new);
	return err;
}
