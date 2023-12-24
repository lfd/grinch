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

#define dbg_fmt(x)	"vmm: " x

#include <asm/irq.h>
#include <asm/isa.h>

#include <grinch/alloc.h>
#include <grinch/const.h>
#include <grinch/errno.h>
#include <grinch/hypercall.h>
#include <grinch/kmm.h>
#include <grinch/printk.h>
#include <grinch/serial.h>
#include <grinch/sbi.h>
#include <grinch/task.h>
#include <grinch/vmm.h>
#include <grinch/pmm.h>
#include <grinch/paging.h>
#include <grinch/vfs.h>
#include <grinch/syscall.h>

#define VM_GPHYS_BASE	(0xa0000000UL)
/*
 * 4MiB for grinch
 * 4KiB for DTB
 */
#define VM_SIZE_RAW	(4 * MIB)
#define VM_PAGES	(PAGES(VM_SIZE_RAW) + 1)
#define VM_FDT_OFFSET	VM_SIZE_RAW

#define GUEST_ROOT_PT_PAGES	(1 << 2)


/* forward no IRQs. We have no guest IRQs at the moment */
#define HIDELEG	0
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

void arch_vmachine_activate(struct vmachine *vm)
{
	enable_mmu_hgatp(satp_mode, kmm_v2p(vm->hv_page_table));

	u64 hstatus =
	        (2ULL << HSTATUS_VSXL_SHIFT) | /* Xlen 64 */
	        /* No TVM */
	        /* No TW */
	        /* No TSR */
	        /* No HU */
	        HSTATUS_SPV | /* activate VMM */
	        0;
	csr_write(CSR_HSTATUS, hstatus);

	// FIXME: With this, we will return to S-Mode inside the VM. It might
	// happen that the VM interrupts in U-Mode. This is not supported at
	// the moment
	csr_set(sstatus, SR_SPP);

	/* Restore shadowed VM registers */
	csr_write(CSR_VSSTATUS, vm->vregs.vsstatus);
	csr_write(CSR_VSATP, vm->vregs.vsatp);
	csr_write(CSR_VSSCRATCH, vm->vregs.vsscratch);
	csr_write(CSR_VSTVEC, vm->vregs.vstvec);
	csr_write(CSR_VSCAUSE, vm->vregs.vscause);
	csr_write(CSR_VSIE, vm->vregs.vsie);
	csr_write(CSR_VSTVAL, vm->vregs.vstval);
}

static inline struct sbiret handle_sbi_base(unsigned long fid)
{
	struct sbiret ret;

	ret.error = SBI_SUCCESS;
	switch (fid) {
		case SBI_EXT_BASE_GET_SPEC_VERSION:
			ret.value = 0x1000000;
			break;

		case SBI_EXT_BASE_GET_IMP_ID:
			ret.value = 1;
			break;

		case SBI_EXT_BASE_GET_IMP_VERSION:
			ret.value = 0x10003;
			break;

		case SBI_EXT_BASE_PROBE_EXT:
		case SBI_EXT_BASE_GET_MVENDORID:
		case SBI_EXT_BASE_GET_MARCHID:
		case SBI_EXT_BASE_GET_MIMPID:
			ret.value = 0;
			break;

		default:
			pr("Base FID %lx not implemented\n", fid);
			ret.error = SBI_ERR_NOT_SUPPORTED;
			ret.value = 0;
			break;
	}

	return ret;
}

static inline struct sbiret handle_sbi_grinch(unsigned long num, unsigned long arg0)
{
	struct sbiret ret;

	switch (num) {
		case GRINCH_HYPERCALL_PRESENT:
			ret.error = SBI_SUCCESS;
			ret.value = current_task()->pid;
			break;

		case GRINCH_HYPERCALL_YIELD:
			ret.error = SBI_SUCCESS;
			ret.value = 0;
			this_per_cpu()->schedule = true;
			break;

		case GRINCH_HYPERCALL_VMQUIT:
			exit(arg0);
			break;

		default:
			pr("Unknown Grinch Hypercall %lx\n", num);
			ret.error = SBI_ERR_NOT_SUPPORTED;
			ret.value = 0;
			break;
	}

	return ret;
}

static int handle_ecall(void)
{
	struct registers *regs;
	unsigned long eid, fid;
	struct sbiret ret;

	regs = &current_task()->regs;
	regs->sepc += 4;

	eid = regs->a7;
	fid = regs->a6;
	switch (eid) { /* EID - Extension ID*/
		case SBI_EXT_0_1_CONSOLE_PUTCHAR:
			uart_write_char(&uart_default, regs->a0);
			ret.error = ret.value = 0;
			break;

		case SBI_EXT_BASE:
			ret = handle_sbi_base(fid);
			break;

		case SBI_EXT_GRNC:
			ret = handle_sbi_grinch(fid, regs->a0);
			break;

		default:
			pr("Extension 0x%lx not implemented\n", eid);
			return -ENOSYS;
	}

	if (eid == SBI_EXT_GRNC && fid == GRINCH_HYPERCALL_VMQUIT)
		return 0;

	regs->a0 = ret.error;
	regs->a1 = ret.value;

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

	task = current_task();
	/* Save regular registers */
	task->regs = *regs;

	if (is_irq(ctx->scause))
		return VMM_FORWARD;

	/* Save VM specific registers */
	vm = task->vmachine;
	vm->vregs.vsstatus = csr_read(CSR_VSSTATUS);
	vm->vregs.vsatp = csr_read(CSR_VSATP);
	vm->vregs.vsscratch = csr_read(CSR_VSSCRATCH);
	vm->vregs.vstvec = csr_read(CSR_VSTVEC);
	vm->vregs.vscause = csr_read(CSR_VSCAUSE);
	vm->vregs.vsie = csr_read(CSR_VSIE);
	vm->vregs.vstval = csr_read(CSR_VSTVAL);

	/* Here we land if we take a trap vom V=1 */
	switch (ctx->scause) {
		case EXC_SUPERVISOR_SYSCALL:
			err = handle_ecall();
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

	ps("Hypervisor Context:\n");
	pr("sstatus: %016lx scause: %016lx\n", ctx->sstatus, ctx->scause);
	pr("hstatus: %016lx\n", ctx->hstatus);
	pr("vsatp: %016lx (phys: %016llx)\n", vsatp, ((vsatp & BIT_MASK(43, 0)) << PAGE_SHIFT));

	return VMM_ERROR;
}

void vmachine_destroy(struct task *task)
{
	int err;
	struct vmachine *vm = task->vmachine;

	if (task == current_task()) {
		csr_write(CSR_HSTATUS, 0);
		disable_mmu_hgatp();
	}

	if (vm->hv_page_table) {
		err = vm_unmap_range(vm->hv_page_table, (void *)VM_GPHYS_BASE, vm->memregion.size);
		if (err)
			panic("vm_unmap_range\n");

		err = kmm_page_free(vm->hv_page_table, GUEST_ROOT_PT_PAGES);
		if (err)
			panic("vmachine_destroy: kmm_free\n");
	}

	if (vm->memregion.size) {
		err = pmm_page_free(vm->memregion.base, PAGES(vm->memregion.size));
		if (err)
			panic("vmachine_destroy: pmm_page_free\n");
	}

	kfree(vm);
}

static int vm_memcpy(struct vmachine *vm, unsigned long offset,
		     const void *src, size_t len)
{
	void *dst;

	if (offset + len > vm->memregion.size)
		return -ERANGE;

	dst = pmm_to_virt(vm->memregion.base) + offset;
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
	struct task *task;
	struct vmachine *vm;
	int err;

	/* Allocate basic structures */
	task = task_alloc_new();
	if (IS_ERR(task))
		return task;

	task->type = GRINCH_VMACHINE;
	task->vmachine = kzalloc(sizeof(*task->vmachine));
	if (!task->vmachine) {
		err = -ENOMEM;
		goto free_out;
	}
	vm = task->vmachine;

	/* Allocate VM specific parts */
	err = pmm_page_alloc_aligned(&vm->memregion.base, VM_PAGES, PAGE_SIZE, 0);
	if (err)
		goto vmfree_out;
	vm->memregion.size = VM_PAGES * PAGE_SIZE;

	ps("Copying kernel...\n");
	err = vm_load_file(vm, "initrd:/kernel.bin", 0);
	if (err)
		goto vmfree_out;

	ps("Copying VM device tree...\n");
	err = vm_load_file(vm, "initrd:/vm.dtb", VM_FDT_OFFSET);
	if (err)
		goto vmfree_out;

	ps("Copying initrd...\n");
	err = vm_memcpy(vm, 1 * MIB, initrd.vbase, initrd.size);
	if (err)
		goto vmfree_out;

	task->regs.a0 = 0;
	task->regs.a1 = VM_GPHYS_BASE + VM_FDT_OFFSET;

	/* setup G-Stage paging */
	vm->hv_page_table =
		kmm_page_zalloc_aligned(GUEST_ROOT_PT_PAGES,
					GUEST_ROOT_PT_PAGES * PAGE_SIZE);
	if (!vm->hv_page_table) {
		err = -ENOMEM;
		goto vmfree_out;
	}

	err = vm_map_range(vm->hv_page_table, (void *)VM_GPHYS_BASE,
			   vm->memregion.base, vm->memregion.size,
			   GRINCH_MEM_RWXU);
	if (err) {
		task_destroy(task);
		return ERR_PTR(err);
	}

	return task;

vmfree_out:
	vmachine_destroy(task);

free_out:
	kfree(task);
	return ERR_PTR(err);
}

int vmm_init(void)
{
	if (!has_hypervisor())
		return -ENOSYS;

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
	csr_write(CSR_HTIMEDELTA, 0);

	return 0;
}

int vm_create_grinch(void)
{
	struct task *task;

	ps("Creating a Grinch VM\n");
	task = vmm_alloc_new();
	if (IS_ERR(task))
		return PTR_ERR(task);

	task->regs.sepc = VM_GPHYS_BASE;
	sched_enqueue(task);

	return 0;
}
