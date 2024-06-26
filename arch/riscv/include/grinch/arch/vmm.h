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

#ifndef _VMM_H
#define _VMM_H

/*
 * To make things easy, let's say that a VM only gets one contiguous memory
 * region.
 */
struct vmachine {
	struct {
		paddr_t base;
		size_t size;
	} memregion;
	page_table_t hv_page_table;

	bool timer_pending;
	struct {
		unsigned long vsstatus;
		unsigned long vsie;
		unsigned long vstvec;
		unsigned long vsscratch;
		unsigned long vscause;
		unsigned long vstval;
		unsigned long hvip;
		unsigned long vsatp;

		/* executes in vs mode? */
		bool vs;
	} vregs;
};

enum vmm_trap_result {
	VMM_HANDLED, /* We had a trap of a VM , and it was successfully addressed */
	VMM_ERROR, /* We're unable to handle the trap. Crash here. */
	VMM_FORWARD, /* We didn't trap from a VM */
};

/* internal routines */
int vmm_handle_ecall(void);

/* external interface */
int vmm_init(void);
enum vmm_trap_result
vmm_handle_trap(struct trap_context *ctx, struct registers *regs);

void vmachine_destroy(struct task *task);

void vmachine_set_timer_pending(struct vmachine *vm);

void arch_vmachine_activate(struct vmachine *vm);

#endif /* _VMM_H */
