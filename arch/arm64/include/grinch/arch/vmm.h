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

struct vmachine {
};

/* external interface */
static inline int vmm_init(void) {return -1;}

static inline void vmachine_destroy(struct task *task) {}

static inline void vmachine_set_timer_pending(struct vmachine *vm) {}

static inline void arch_vmachine_activate(struct vmachine *vm) {}

static inline int vm_create_grinch(void) {return -1;}

static inline void arch_vmachine_save(struct vmachine *vm) {}
static inline void arch_vmachine_restore(struct vmachine *vm) {}

#endif /* _VMM_H */
