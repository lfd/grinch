/*
 *
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

#ifndef _PROCESS_H
#define _PROCESS_H

#include <grinch/fs.h>
#include <grinch/vma.h>

/*
 * At the moment, a process can only hold 10 file descriptors. If we ever need
 * more, we can address it.
 */
#define MAX_FDS	10

struct process {
	struct mm mm;

	struct file_handle fds[MAX_FDS];
};

struct task *process_alloc_new(void);
void process_destroy(struct task *task);

int process_from_fs(struct task *task, const char *pathname);

/* Arch specific routines */
void arch_process_activate(struct process *task);

#endif /* _PROCESS_H */
