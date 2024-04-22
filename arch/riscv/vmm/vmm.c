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

#define dbg_fmt(x)	"vmm: " x

#include <asm/irq.h>
#include <asm/isa.h>

#include <grinch/alloc.h>
#include <grinch/gfp.h>
#include <grinch/fs/initrd.h>
#include <grinch/fs/vfs.h>
#include <grinch/panic.h>
#include <grinch/paging.h>
#include <grinch/printk.h>
#include <grinch/platform.h>
#include <grinch/task.h>
#include <grinch/vsprintf.h>

#include <grinch/arch/vmm.h>

#define VM_GPHYS_BASE	(0xa0000000UL)
/*
 * 4MiB for grinch
 * 4KiB for DTB
 */
#define VM_SIZE_RAW	(4 * MIB)
#define VM_PAGES	(PAGES(VM_SIZE_RAW) + 1)
#define VM_FDT_OFFSET	VM_SIZE_RAW

#define GUEST_ROOT_PT_PAGES	(1 << 2)

#define RISCV_INST_WFI	0x10500073

#define HIDELEG					\
	((IE_SIE << VSIP_TO_HVIP_SHIFT) |	\
	(IE_TIE << VSIP_TO_HVIP_SHIFT) |	\
	(IE_EIE << VSIP_TO_HVIP_SHIFT))

#define HEDELEG					\
	((1UL << EXC_INST_MISALIGNED) |		\
	(1UL << EXC_INST_ACCESS) |		\
	(1UL << EXC_INST_ILLEGAL) |		\
	(1UL << EXC_BREAKPOINT) |		\
	(1UL << EXC_LOAD_ACCESS_MISALIGNED) |	\
	(1UL << EXC_LOAD_ACCESS) |		\
	(1UL << EXC_AMO_ADDRESS_MISALIGNED) |	\
	(1UL << EXC_STORE_ACCESS) |		\
	(1UL << EXC_SYSCALL) |			\
	(1UL << EXC_INST_PAGE_FAULT) |		\
	(1UL << EXC_LOAD_PAGE_FAULT) |		\
	(1UL << EXC_STORE_PAGE_FAULT))

void vmachine_set_timer_pending(struct vmachine *vm)
{
	vm->timer_pending = true;
}

void arch_vmachine_save(struct vmachine *vm)
{
	u64 sstatus;

	vm->vregs.vsstatus = csr_read(CSR_VSSTATUS);
	vm->vregs.vsie = csr_read(CSR_VSIE);
	vm->vregs.vstvec = csr_read(CSR_VSTVEC);
	vm->vregs.vsscratch = csr_read(CSR_VSSCRATCH);
	vm->vregs.vscause = csr_read(CSR_VSCAUSE);
	vm->vregs.vstval = csr_read(CSR_VSTVAL);
	vm->vregs.hvip = csr_read(CSR_HVIP);
	vm->vregs.vsatp = csr_read(CSR_VSATP);

	sstatus = csr_read(sstatus);
	vm->vregs.vs = !!(sstatus & SR_SPP);
}

void arch_vmachine_restore(struct vmachine *vm)
{
	if (vm->vregs.vs)
		csr_set(sstatus, SR_SPP);
	else
		csr_clear(sstatus, SR_SPP);

	if (vm->timer_pending) {
		vm->timer_pending = false;
		vm->vregs.hvip |= VIE_TIE;
	}

	/* Restore shadowed VM registers */
	csr_write(CSR_VSSTATUS, vm->vregs.vsstatus);
	csr_write(CSR_VSIE, vm->vregs.vsie);
	csr_write(CSR_VSTVEC, vm->vregs.vstvec);
	csr_write(CSR_VSSCRATCH, vm->vregs.vsscratch);
	csr_write(CSR_VSCAUSE, vm->vregs.vscause);
	csr_write(CSR_VSTVAL, vm->vregs.vstval);
	csr_write(CSR_HVIP, vm->vregs.hvip);
	csr_write(CSR_VSATP, vm->vregs.vsatp);
}

void arch_vmachine_activate(struct vmachine *vm)
{
	enable_mmu_hgatp(hgatp_mode, v2p(vm->hv_page_table));

	u64 hstatus =
	        (2ULL << HSTATUS_VSXL_SHIFT) | /* Xlen 64 */
		HSTATUS_VTW |
	        /* No TVM */
	        /* No TW */
	        /* No TSR */
	        /* No HU */
	        HSTATUS_SPV | /* activate VMM */
	        0;
	csr_write(CSR_HSTATUS, hstatus);
}

static inline u16 gmem_read16(unsigned long addr)
{
	u64 mem;

	/*
	 * hlvx.hu can potentially fault and throw an exception. But if we end
	 * up here, we're decoding an instruction that the guest was possible
	 * to execute. Hence, it must be backed by existing memory, and no
	 * exception can occur.
	 */
	asm volatile(".insn r 0x73, 0x4, 0x32, %0, %1, x3\n" /* hlvx.hu */
		     : "=r"(mem) : "r"(addr) : "memory");

	return mem;
}

static int vmm_handle_inst(void)
{
	struct registers *regs;
	bool is_compressed;
	u32 instruction;

	regs = &current_task()->regs;
	/* Ensure the instruction is 16-bit aligned */
	if (regs->pc & 0x1)
		return -EINVAL;

	/* Load the faulting instruction */
	instruction = gmem_read16(regs->pc);
	if ((instruction & 0x3) == 0x3) {
		is_compressed = false;
		instruction |= (u32)gmem_read16(regs->pc + 2) << 16;
	} else
		is_compressed = true;

	if (instruction != RISCV_INST_WFI)
		return -ENOSYS;

	/* we have a WFI instruction */
	if (!current_task()->vmachine.vregs.hvip)
		task_set_wfe(current_task());

	this_per_cpu()->schedule = true;

	regs->pc += is_compressed ? 2 : 4;

	return 0;
}

enum vmm_trap_result
vmm_handle_trap(struct trap_context *ctx, struct registers *regs)
{
	struct task *task;
	struct vmachine *vm;
	unsigned long vsatp;
	int err;

	ctx->hstatus = csr_read(CSR_HSTATUS);
	/* check if we arrive from a guest */
	if (!(ctx->hstatus & HSTATUS_SPV))
		return VMM_FORWARD;

	/* Was the VM in VU-Mode? */
	if (!(ctx->sstatus & SR_SPP))
		// Why does sfence.vma trap from VU->HS directly?
		BUG();

	task = current_task();
	/* Save regular registers */
	task->regs = *regs;

	/* Save VM specific registers */
	vm = &task->vmachine;
	arch_vmachine_save(vm);

	/* Here we land if we take a trap vom V=1 */
	switch (ctx->scause) {
		case EXC_SUPERVISOR_SYSCALL:
			err = vmm_handle_ecall();
			if (err)
				goto out;
			break;

		case EXC_VIRTUAL_INST_FAULT:
			err = vmm_handle_inst();
			if (err)
				goto out;
			break;

		default:
			pr("Unknown Trap in Hypervisor taken\n");
			goto out;
	}

	return VMM_HANDLED;

out:
	vsatp = csr_read(vsatp);

	pr("Hypervisor Context:\n");
	pr("SSTATUS: %016lx SCAUSE: %016lx\n", ctx->sstatus, ctx->scause);
	pr("HSTATUS: %016lx  HTVAL: %016lx\n",
		ctx->hstatus, csr_read(CSR_HTVAL));
	pr("VSATP: %016lx (phys: %016llx)\n", vsatp, ((vsatp & BIT_MASK(43, 0)) << PAGE_SHIFT));

	return VMM_ERROR;
}

void vmachine_destroy(struct task *task)
{
	struct vmachine *vm;
	int err;

	if (task == current_task()) {
		csr_write(CSR_HSTATUS, 0);
		disable_mmu_hgatp();
	}

	vm = &task->vmachine;
	if (vm->hv_page_table) {
		err = vm_unmap_range(vm->hv_page_table, (void *)VM_GPHYS_BASE, vm->memregion.size);
		if (err)
			panic("vm_unmap_range\n");

		err = free_pages(vm->hv_page_table, GUEST_ROOT_PT_PAGES);
		if (err)
			panic("vmachine_destroy: free_pages\n");
	}

	if (vm->memregion.size) {
		err = phys_free_pages(vm->memregion.base, PAGES(vm->memregion.size));
		if (err)
			panic("vmachine_destroy: pmm_page_free\n");
	}
}

static int vm_memcpy(struct vmachine *vm, unsigned long offset,
		     const void *src, size_t len)
{
	void *dst;

	if (offset + len > vm->memregion.size)
		return -ERANGE;

	dst = p2v(vm->memregion.base) + offset;
	memcpy(dst, src, len);

	return 0;
}

static int vm_load_file(struct vmachine *vm, const char *filename, size_t offset)
{
	size_t len;
	void *file;
	int err;

	if (offset >= vm->memregion.size)
		return -ERANGE;

	file = vfs_read_file(filename, &len);
	if (IS_ERR(file)) {
		pr("Unable to read from VFS: %s\n", filename);
		return PTR_ERR(file);
	}

	if (len > vm->memregion.size - offset) {
		kfree(file);
		return -ENOMEM;
	}

	err = vm_memcpy(vm, offset, file, len);
	kfree(file);

	return err;
}

static struct task *vmm_alloc_new(void)
{
	struct task *task, *parent;
	struct vmachine *vm;
	char buf[128];
	int err;

	/* Allocate basic structures */
	task = task_alloc_new("GrinchVM");
	if (IS_ERR(task))
		return task;

	parent = current_task();
	spin_lock(&parent->lock);

	list_add(&task->sibling, &parent->children);
	task->parent = parent;

	task->type = GRINCH_VMACHINE;
	memset(&task->vmachine, 0, sizeof(task->vmachine));
	vm = &task->vmachine;

	/* Allocate VM specific parts */
	err = phys_pages_alloc_aligned(&vm->memregion.base, VM_PAGES, PAGE_SIZE);
	if (err)
		goto vmfree_out;
	vm->memregion.size = VM_PAGES * PAGE_SIZE;

	pr_dbg("Copying kernel...\n");
	err = vm_load_file(vm, "/initrd/kernel.bin", 0);
	if (err)
		goto vmfree_out;

	pr_dbg("Copying VM device tree...\n");
	snprintf(buf, sizeof(buf), "/initrd/dtb/%s.dtb", platform_model);
	err = vm_load_file(vm, buf, VM_FDT_OFFSET);
	if (err)
		goto vmfree_out;

	pr_dbg("Copying initrd...\n");
	err = vm_memcpy(vm, 1 * MIB, initrd.vbase, initrd.size);
	if (err)
		goto vmfree_out;

	task->regs.a0 = 0;
	task->regs.a1 = VM_GPHYS_BASE + VM_FDT_OFFSET;
	vm->vregs.vs = true;

	/* setup G-Stage paging */
	vm->hv_page_table =
		zalloc_pages_aligned(GUEST_ROOT_PT_PAGES,
				     GUEST_ROOT_PT_PAGES * PAGE_SIZE);
	if (!vm->hv_page_table) {
		err = -ENOMEM;
		goto vmfree_out;
	}

	err = vm_map_range(vm->hv_page_table, (void *)VM_GPHYS_BASE,
			   vm->memregion.base, vm->memregion.size,
			   GRINCH_MEM_RWXU);
	if (err)
		goto vmfree_out;

	spin_unlock(&parent->lock);
	return task;

vmfree_out:
	spin_unlock(&parent->lock);
	task_exit(task, err);
	task_destroy(task);

	return ERR_PTR(err);
}

static void __init vmm_cpu_init(void *)
{
	/* These are always the same across VMs */
	csr_write(CSR_HEDELEG, HEDELEG);
	csr_write(CSR_HIDELEG, HIDELEG);
	/* disable IRQs */
	csr_write(CSR_HVIP, 0);
	csr_write(CSR_HIP, 0);
	csr_write(CSR_HIE, 0);
	csr_write(CSR_HGEIE, 0);
	/* Allow rdtime */
	csr_write(CSR_HCOUNTEREN, HCOUNTEREN_TM);
	// What the heck?!
	//csr_write(CSR_HTIMEDELTA, 0);
	csr_write(CSR_HENVCFG, 0);
}

int __init vmm_init(void)
{
	if (!has_hypervisor())
		return -ENOSYS;

	on_each_cpu(vmm_cpu_init, NULL);

	return 0;
}

int vm_create_grinch(void)
{
	struct task *task;

	if (!has_hypervisor())
		return -ENOSYS;

	task = vmm_alloc_new();
	if (IS_ERR(task))
		return PTR_ERR(task);

	task->regs.pc = VM_GPHYS_BASE;
	task->state = TASK_RUNNABLE;
	sched_enqueue(task);

	return task->pid;
}
