/*
 *
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

#ifndef _PROCESS_H
#define _PROCESS_H

#include <grinch/vma.h>

struct process {
	struct mm mm;
};

struct task *process_alloc_new(void);
void process_destroy(struct process *p);

int process_from_fs(struct task *task, const char *pathname);

/* Arch specific routines */
void arch_process_activate(struct process *task);

#endif /* _PROCESS_H */
