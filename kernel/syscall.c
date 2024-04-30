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

#define dbg_fmt(x)	"syscall: " x

#include <grinch/errno.h>
#include <grinch/syscall.h>
#include <grinch/task.h>

#include <generated/syscall.h>

#include "syscall_table.c"

void syscall(unsigned long no, struct syscall_args *args)
{
	syscall_stub_t sysfun;
	struct task *cur;
	long ret;

	cur = current_task();
	if (cur->state != TASK_RUNNING)
		BUG();

	sysfun = NULL;
	if (no < ARRAY_SIZE(syscalls)) {
		sysfun = syscalls[no];
	} else if (no >= SYS_grinch_base) {
		no -= SYS_grinch_base;
		if (no >= ARRAY_SIZE(grinch_calls)) {
			ret = -ENOSYS;
			goto sys_out;
		}

		sysfun = grinch_calls[no];
	}

	if (sysfun)
		ret = sysfun(args);
	else
		ret = -ENOSYS;

	/*
	 * 1. On errors, always set the return value
	 * 2. We have special treatments for exit,execve,wait
	 * 3. Syscalls might have killed the task. Check for its existence
	 */
sys_out:
	cur = current_task();
	if ((ret < 0 ||
	    !(no == SYS_exit || no == SYS_execve || no == SYS_wait)) && cur)
		regs_set_retval(&cur->regs, ret);
}
