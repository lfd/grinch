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

#define dbg_fmt(x)	"process: " x

#include <grinch/align.h>
#include <grinch/alloc.h>
#include <grinch/asid.h>
#include <grinch/device.h>
#include <grinch/elf.h>
#include <grinch/errno.h>
#include <grinch/fs/util.h>
#include <grinch/fs/vfs.h>
#include <grinch/gcall.h>
#include <grinch/gconfig.h>
#include <grinch/gfp.h>
#include <grinch/pci.h>
#include <grinch/printk.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>
#include <grinch/percpu.h>
#include <grinch/syscall.h>
#include <grinch/ttp.h>

#define ARG_MAX		PAGE_SIZE

#define VMA_NAME_HEAP	"[heap]"

struct auxv {
	unsigned long tag;
	unsigned long value;
} __packed;

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

/* All of it goes on the new stack, where there is only so much room */
static int uenv_check(const struct uenv_array *argv,
		      const struct uenv_array *envp)
{
	size_t size;

	size = uenv_sz(argv) + uenv_sz(envp) + sizeof(unsigned long) +
	       2 * sizeof(struct auxv);
	if (size > ARG_MAX)
		return -E2BIG;

	return 0;
}

static void kinfo_init(struct kinfo *kinfo)
{
	kinfo->wall_base = wall_base;
	arch_kinfo_init(kinfo);
}

/* Does a range of size bytes at addr lie inside len bytes at base? */
static bool elf_within(uintptr_t base, size_t len, uintptr_t addr, size_t size)
{
	if (addr < base || addr - base > len)
		return false;

	return size <= len - (addr - base);
}

/*
 * An image is only as long as it was read, and everything it says about itself
 * has to fit in that. Whoever walks it afterwards may take that for granted.
 */
static int elf_check_bounds(const Elf_Ehdr *ehdr, size_t len)
{
	const Elf_Phdr *phdr;
	unsigned int d;

	if (len < sizeof(*ehdr))
		return -EINVAL;

	if (ehdr->e_phentsize != sizeof(*phdr))
		return -EINVAL;

	/*
	 * The table is walked in place, so it must lie as the walk reads it:
	 * a misaligned load is the firmware's mercy at best, a trap at worst.
	 */
	if (!IS_ALIGNED(ehdr->e_phoff, __alignof__(*phdr)))
		return -EINVAL;

	if (!elf_within(0, len, ehdr->e_phoff,
			(size_t)ehdr->e_phnum * sizeof(*phdr)))
		return -EINVAL;

	phdr = (const Elf_Phdr *)((const void *)ehdr + ehdr->e_phoff);
	for (d = 0; d < ehdr->e_phnum; d++, phdr++) {
		if (!elf_within(0, len, phdr->p_offset, phdr->p_filesz))
			return -EINVAL;

		if (phdr->p_type != PT_LOAD)
			continue;

		/* A part cannot bring more along than it makes room for */
		if (phdr->p_filesz > phdr->p_memsz)
			return -EINVAL;

		/* A part that makes room for nothing is no part at all */
		if (!phdr->p_memsz)
			return -EINVAL;

		/* Nor may the pages it takes reach around the address space */
		if (phdr->p_vaddr + page_up(phdr->p_memsz) <= phdr->p_vaddr)
			return -EINVAL;
	}

	return 0;
}

/*
 * An image is one piece: its parts only work at the distance from each other
 * that they were linked at, so where they may go is decided once, for all of
 * them. Where they may not move at all, the answer is where they asked to be.
 */
static int process_place(struct task *task, Elf_Ehdr *ehdr, uintptr_t *bias)
{
#ifdef CONFIG_MMU
	*bias = 0;

	return 0;
#else
	uintptr_t lo, hi;
	Elf_Phdr *phdr;
	struct vma *vma;
	unsigned int d;
	size_t size;

	lo = -1;
	hi = 0;
	phdr = (Elf_Phdr *)((void *)ehdr + ehdr->e_phoff);
	for (d = 0; d < ehdr->e_phnum; d++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_vaddr < lo)
			lo = phdr->p_vaddr;
		if (phdr->p_vaddr + phdr->p_memsz > hi)
			hi = phdr->p_vaddr + phdr->p_memsz;
	}

	if (lo >= hi)
		return -EINVAL;

	lo &= PAGE_MASK;
	size = page_up(hi - lo);

	/* Rounded up, the whole of it must still fit in the address space */
	if (lo + size <= lo)
		return -EINVAL;

	vma = uvma_create(task, (void *)lo, size,
			  VMA_FLAG_USER | VMA_FLAG_RW | VMA_FLAG_X, "[image]");
	if (IS_ERR(vma))
		return PTR_ERR(vma);

	*bias = (uintptr_t)vma->base - lo;

	return 0;
#endif
}

#ifdef CONFIG_USER_PIE
/* Find a link time range in the image we were handed, whole or not at all. */
static void *elf_at(Elf_Ehdr *ehdr, uintptr_t addr, size_t size)
{
	Elf_Phdr *phdr;
	unsigned int d;

	phdr = (Elf_Phdr *)((void *)ehdr + ehdr->e_phoff);
	for (d = 0; d < ehdr->e_phnum; d++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		if (!elf_within(phdr->p_vaddr, phdr->p_filesz, addr, size))
			continue;

		return (void *)ehdr + phdr->p_offset + (addr - phdr->p_vaddr);
	}

	return NULL;
}

/*
 * Every word the image holds that names an address was left blank, to be
 * filled in once the image knows where it is.
 */
static int process_relocate(struct task *task, Elf_Ehdr *ehdr, uintptr_t bias)
{
	uintptr_t table, value;
	unsigned int d, entries;
	size_t size, copied;
	Elf_Phdr *phdr;
	Elf_Rela *rela;
	Elf_Dyn *dyn;

	dyn = NULL;
	entries = 0;
	phdr = (Elf_Phdr *)((void *)ehdr + ehdr->e_phoff);
	for (d = 0; d < ehdr->e_phnum; d++, phdr++)
		if (phdr->p_type == PT_DYNAMIC) {
			dyn = (Elf_Dyn *)((void *)ehdr + phdr->p_offset);
			entries = phdr->p_filesz / sizeof(*dyn);
		}

	/* An image that may not be moved says nothing about itself. */
	if (!dyn)
		return 0;

	table = 0;
	size = 0;
	for (; entries && dyn->d_tag != DT_NULL; entries--, dyn++) {
		if (dyn->d_tag == DT_RELA)
			table = dyn->d_un.d_ptr;
		else if (dyn->d_tag == DT_RELASZ)
			size = dyn->d_un.d_val;
	}

	if (!table || !size)
		return 0;

	rela = elf_at(ehdr, table, size);
	if (!rela)
		return -EINVAL;

	for (entries = size / sizeof(*rela); entries; entries--, rela++) {
		/* The linker leaves what it dropped in place, to be skipped. */
		if (ELF_R_TYPE(rela->r_info) == ELF_R_NONE)
			continue;

		if (ELF_R_TYPE(rela->r_info) != ELF_R_RELATIVE) {
			pr_warn("Unhandled relocation type %lu\n",
				(unsigned long)ELF_R_TYPE(rela->r_info));
			return -ENOSYS;
		}

		value = bias + rela->r_addend;
		copied = copy_to_user(task,
				      (void __user *)(uintptr_t)(rela->r_offset + bias),
				      &value, sizeof(value));
		if (copied != sizeof(value))
			return -EFAULT;
	}

	return 0;
}
#else
static inline int
process_relocate(struct task *task, Elf_Ehdr *ehdr, uintptr_t bias)
{
	return 0;
}
#endif

static int process_load_elf(struct task *task, Elf_Ehdr *ehdr,
			    const struct uenv_array *argv,
			    const struct uenv_array *envp)
{
	void __user *stack_top, __user *tmp, *src, *base;
	char __user *uargv_string, *uenvp_string;
	unsigned long argc, copied;
	unsigned int d, vma_flags;
	struct kinfo kinfo;
	struct auxv aux[2];
	struct vma *vma;
	size_t vma_size;
	Elf_Phdr *phdr;
	uintptr_t bias;
	int err;

	/* Prepare user stack */
	vma_flags = VMA_FLAG_USER | VMA_FLAG_RW | VMA_FLAG_LAZY;
	vma = uvma_create(task, (void *)USER_STACK_BOTTOM, USER_STACK_SIZE,
			  vma_flags, "[stack]");
	if (IS_ERR(vma))
		return PTR_ERR(vma);

	stack_top = vma->base + vma->size;

	kinfo_init(&kinfo);
	stack_top = PTR_ALIGN_DOWN(stack_top - sizeof(kinfo), 8);
	copied = copy_to_user(task, stack_top, &kinfo, sizeof(kinfo));
	if (copied != sizeof(kinfo))
		return -ENOMEM;

	aux[0].tag = AT_KINFO;
	aux[0].value = (uintptr_t)stack_top;
	aux[1].tag = AT_NULL;
	aux[1].value = 0;

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
	stack_top -= (uenv_elems(envp) + uenv_elems(argv) + 1) * BYTES_PER_LONG + sizeof(aux);
	stack_top = PTR_ALIGN_DOWN(stack_top, 8);

	tmp = stack_top;
	/* store argc */
	argc = argv ? argv->elements : 0;
	copied = copy_to_user(task, tmp, &argc, sizeof(argc));
	if (copied != sizeof(argc))
		return -ENOMEM;
	tmp += sizeof(argc);

	tmp = fill_uenv_table(task, argv, tmp, uargv_string);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	tmp = fill_uenv_table(task, envp, tmp , uenvp_string);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	if (sizeof(aux)) {
		copied = copy_to_user(task, tmp, aux, sizeof(aux));
		if (copied != sizeof(aux))
			return -ENOMEM;
	}

	err = process_place(task, ehdr, &bias);
	if (err)
		return err;

	/* Load process */
	phdr = (Elf_Phdr*)((void*)ehdr + ehdr->e_phoff);
	task->process.brk.base = NULL;
	for (d = 0; d < ehdr->e_phnum; d++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		base = (void *)(uintptr_t)(phdr->p_vaddr + bias);
		vma_size = page_up(phdr->p_memsz);

#ifdef CONFIG_MMU
		vma_flags = VMA_FLAG_USER;
		if (phdr->p_flags & PF_R)
			vma_flags |= VMA_FLAG_R;
		if (phdr->p_flags & PF_W)
			vma_flags |= VMA_FLAG_W;
		if (phdr->p_flags & PF_X)
			vma_flags |= VMA_FLAG_X;

		/* The region must not collide with any other VMA */
		if (uvma_collides(&task->process, base, vma_size))
			return -EINVAL;

		vma = uvma_create(task, base, vma_size, vma_flags, NULL);
		if (IS_ERR(vma))
			return PTR_ERR(vma);

		base = vma->base;
#endif
		src = (void *)ehdr + phdr->p_offset;
		copied = copy_to_user(task, base, src, phdr->p_filesz);
		if (copied != phdr->p_filesz)
			return -ERANGE;

		if (base + vma_size > task->process.brk.base)
			task->process.brk.base = base + vma_size;
	}

	err = process_relocate(task, ehdr, bias);
	if (err)
		return err;

#ifndef CONFIG_MMU
	/*
	 * A heap has nowhere to grow into once the program runs, as whatever
	 * follows it is already spoken for, so it is given its size here. The
	 * break then only moves within what it was given.
	 */
	static_assert(IS_ALIGNED(CONFIG_USER_HEAP_SIZE, PAGE_SIZE),
		      "the heap must be whole pages");
	vma = uvma_create(task, task->process.brk.base, CONFIG_USER_HEAP_SIZE,
			  VMA_FLAG_USER | VMA_FLAG_RW, VMA_NAME_HEAP);
	if (IS_ERR(vma))
		return PTR_ERR(vma);

	task->process.brk.base = vma->base;
	task->process.brk.cur = vma->base;
	task->process.brk.vma = vma;
#endif

	task_set_context(task, ehdr->e_entry + bias, (uintptr_t)stack_top);

	return 0;
}

/*
 * Fetch a program, and refuse it here for everything it can be refused for:
 * this is the last point at which there is still a caller to tell.
 */
static void *process_fetch_elf(struct file *at, const char *pathname)
{
	const Elf_Ehdr *ehdr;
	struct file *file;
	size_t len;
	void *elf;
	int err;

	file = file_open_at(at, pathname);
	if (IS_ERR(file))
		return file;

	elf = vfs_read_file(file, &len);
	file_close(file);
	if (IS_ERR(elf))
		return elf;

	/* Before anything is read from it, it has to be long enough to read */
	ehdr = elf;
	err = elf_check_bounds(ehdr, len);
	if (err)
		goto free_out;

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) {
		err = trace_error(-EINVAL);
		goto free_out;
	}

	if (ehdr->e_machine != ELF_ARCH) {
		err = -EINVAL;
		goto free_out;
	}

	return elf;

free_out:
	kfree(elf);
	return ERR_PTR(err);
}

int process_from_path(struct task *task, struct file *at, const char *pathname,
		      struct uenv_array *argv, struct uenv_array *envp)
{
	void *elf;
	int err;

	err = uenv_check(argv, envp);
	if (err)
		return err;

	elf = process_fetch_elf(at, pathname);
	if (IS_ERR(elf))
		return PTR_ERR(elf);

	err = process_load_elf(task, elf, argv, envp);
	kfree(elf);

	return err;
}

/*
 * Put a program in the place of the one a task is already running. Nothing is
 * given up until the new one is in hand; past that there is no image left to
 * carry an error back to, so what fails from there on ends the task.
 */
static int process_replace(struct task *task, struct file *at,
			   const char *pathname, struct uenv_array *argv,
			   struct uenv_array *envp)
{
	struct process *process;
	void *elf;
	int err;

	process = &task->process;

	err = uenv_check(argv, envp);
	if (err)
		return err;

	elf = process_fetch_elf(at, pathname);
	if (IS_ERR(elf))
		return PTR_ERR(elf);

	task_set_name(task, argv->elements ? argv->string : "NO NAME");

	uvmas_destroy(process);
	process->brk.base = NULL;
	process->brk.vma = NULL;

	err = process_load_elf(task, elf, argv, envp);
	kfree(elf);
	if (err) {
		pr("execve failed on task %u: %pe\n", task->pid, ERR_PTR(err));
		task_exit(task, err);
	}

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
		if (process->fds[i]) {
			file_handle_put(process->fds[i]);
			process->fds[i] = NULL;
		}

	uvmas_destroy(process);

	if (process->cwd.pathname) {
		file_close(process->cwd.file);
		process->cwd.file = NULL;

		kfree(process->cwd.pathname);
		process->cwd.pathname = NULL;
	}

#ifdef CONFIG_MMU
	if (process->mm.page_table) {
		/* The dying process' page table may be the live root */
		if (this_per_cpu()->current_task == task)
			arch_process_deactivate();
		free_pages(process->mm.page_table, 1);
	}

	asid_free(process->mm.asid);
#endif
}

struct task *process_alloc_new(const char *name)
{
	struct task *task;

	task = task_alloc_new(name);
	if (IS_ERR(task))
		return task;

	task->type = GRINCH_PROCESS;
#ifdef CONFIG_MMU
	task->process.mm.page_table = zalloc_pages(1);
	if (!task->process.mm.page_table) {
		kfree(task);
		return ERR_PTR(-ENOMEM);
	}

	task->process.mm.asid = asid_alloc();

	arch_mm_init(&task->process.mm);
#endif

	INIT_LIST_HEAD(&task->process.mm.vmas);

	return task;
}

/*
 * Hand every open file of one process to another. Both end up naming the same
 * open file descriptions, so they move each offset together. The caller holds
 * the lock of the task the files come from, so the set is taken whole.
 */
void process_dup_fds(struct task *from, struct task *to)
{
	struct file_handle *fh;
	int fd;

	for (fd = 0; fd < MAX_FDS; fd++) {
		fh = from->process.fds[fd];
		if (fh) {
			file_handle_get(fh);
			to->process.fds[fd] = fh;
		}
	}
}

int process_handle_fault(struct task *task, void __user *addr, bool is_write)
{
	struct vma *vma;
	int err;

	/* Nothing is filled in on demand where nothing can fault. */
	if (!IS_ENABLED(CONFIG_MMU))
		return -EFAULT;

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

	return err;
}

SYSCALL_DEF3(execve, const char __user *, _pathname,
	       const char __user *const __user *, uargv,
	       const char __user *const __user *, uenvp)
{
	struct uenv_array argv, envp;
	struct task *this;
	char *pathname;
	int err;

	this = current_task();

	pathname = pathname_from_user(_pathname, NULL);
	if (IS_ERR(pathname))
		return PTR_ERR(pathname);

	err = uenv_dup(this, uargv, &argv);
	if (err)
		goto pathname_out;

	err = uenv_dup(this, uenvp, &envp);
	if (err)
		goto uargv_free_out;

	err = process_replace(this, cwd(), pathname, &argv, &envp);

	uenv_free(&envp);

uargv_free_out:
	uenv_free(&argv);

pathname_out:
	kfree(pathname);

	return err;
}

/*
 * Start a program as a task of its own. Nothing is inherited that would have
 * to be placed twice: the new task is given an address space of its own and
 * the program is loaded straight into it, so this asks nothing of translation
 * and works where a process cannot be duplicated at all.
 */
SYSCALL_DEF3(grinch_spawn, const char __user *, _pathname,
	     const char __user *const __user *, uargv,
	     const char __user *const __user *, uenvp)
{
	struct task *this, *new;
	struct uenv_array argv, envp;
	char *pathname;
	int err;

	this = current_task();

	pathname = pathname_from_user(_pathname, NULL);
	if (IS_ERR(pathname))
		return PTR_ERR(pathname);

	err = uenv_dup(this, uargv, &argv);
	if (err)
		goto pathname_out;

	err = uenv_dup(this, uenvp, &envp);
	if (err)
		goto uargv_free_out;

	new = process_alloc_new(argv.elements ? argv.string : pathname);
	if (IS_ERR(new)) {
		err = PTR_ERR(new);
		goto uenvp_free_out;
	}

	new->parent = this;

	spin_lock(&this->lock);
	process_dup_fds(this, new);
	spin_unlock(&this->lock);

	/*
	 * Loading touches the filesystem, so it runs unlocked: until the task
	 * is enqueued, no one but us can reach it, and what it inherits from
	 * us cannot change, as we are here and not at chdir or execve.
	 */
	err = process_setcwd(new, this->process.cwd.pathname);
	if (err)
		goto destroy_out;

	/* Resolved against our own directory, as the child has just taken it */
	err = process_from_path(new, cwd(), pathname, &argv, &envp);
	if (err)
		goto destroy_out;

	spin_lock(&this->lock);
	spin_lock(&new->lock);
	new->state = TASK_RUNNABLE;
	list_add(&new->sibling, &this->children);
	spin_unlock(&new->lock);
	spin_unlock(&this->lock);

	task_enqueue(new);
	sched_all();

	err = new->pid;
	goto uenvp_free_out;

destroy_out:
	task_exit(new, err);
	task_put(new);

uenvp_free_out:
	uenv_free(&envp);

uargv_free_out:
	uenv_free(&argv);

pathname_out:
	kfree(pathname);

	return err;
}

SYSCALL_DEF1(brk, unsigned long, addr)
{
	struct process *process;
	unsigned long base, brk;
	struct task *task;
#ifdef CONFIG_MMU
	unsigned int vma_flags;
	struct vma *vma_heap;
	size_t size;
#endif

	task = current_task();
	process = &task->process;

	base = (unsigned long)process->brk.base;

	spin_lock(&task->lock);
	if (!addr)
		goto report_out;

	/* We only support page-wise changes of the program break */
	if (addr % PAGE_SIZE) {
		brk = -EINVAL;
		goto unlock_out;
	}

	/* We can not shift break to the left */
	if (addr < base) {
		brk = -EINVAL;
		goto unlock_out;
	}

#ifdef CONFIG_MMU
	size = addr - base;

	/* Zero-size VMAs are not allowed */
	if (size == 0) {
		brk = -EINVAL;
		goto unlock_out;
	}

	if (!process->brk.vma) {
		vma_flags = VMA_FLAG_USER | VMA_FLAG_RW | VMA_FLAG_LAZY;
		vma_heap = uvma_create(task, process->brk.base, size, vma_flags, VMA_NAME_HEAP);
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
#else
	/* The heap has all it will get: the break only moves inside it. */
	if (addr > base + process->brk.vma->size) {
		brk = -ENOMEM;
		goto unlock_out;
	}

	process->brk.cur = (void __user *)addr;
#endif

report_out:
#ifdef CONFIG_MMU
	brk = base + (process->brk.vma ? process->brk.vma->size : 0);
#else
	brk = (unsigned long)process->brk.cur;
#endif

unlock_out:
	spin_unlock(&task->lock);
	return brk;
}

SYSCALL_DEF2(nanosleep, const struct timespec __user *, _req,
	     struct timespec __user *, rem)
{
	struct timespec req;
	unsigned long ret;
	struct task *t;

	t = current_task();
	ret = copy_from_user(t, &req, _req, sizeof(req));
	if (ret != sizeof(req))
		return -EFAULT;

	task_sleep_for(t, &req);
	this_per_cpu()->schedule = true;

	if (rem)
		if (umemset(t, rem, 0, sizeof(rem)) != sizeof(rem))
			return -EFAULT;

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
#ifdef CONFIG_PCI
			pci_lspci();
#else
			ret = -ENOSYS;
#endif
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
