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

#define dbg_fmt(x)	"syscall: " x

#include <grinch/compiler_attributes.h>
#include <grinch/errno.h>
#include <grinch/printk.h>
#include <grinch/syscall.h>
#include <grinch/task.h>

#include <generated/syscall.h>

static __used long syscall_unavailable(struct syscall_args *args)
{
	pr_dbg("unavailable syscall invoked\n");
	return -ENOSYS;
}

#define cond_syscall(name)					\
	asm(".weak ___sys_" #name "\n\t"			\
	    ".set  ___sys_" #name ", syscall_unavailable")

cond_syscall(grinch_create_grinch_vm);

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
	 * 1. A handler may have left the return value to someone else
	 * 2. Syscalls might have killed the task; the CPU then fell back to
	 *    its idle task, whose registers are nobody's to write.
	 */
sys_out:
	if (ret == SYSCALL_NO_RETVAL)
		return;

	cur = current_task();
	if (!task_is_idle(cur))
		regs_set_retval(&cur->regs, ret);
}
