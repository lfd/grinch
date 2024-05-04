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
#include <grinch/syscall_common.h>
#include <grinch/task.h>

void syscall(unsigned long no, unsigned long arg1,
	     unsigned long arg2, unsigned long arg3,
	     unsigned long arg4, unsigned long arg5,
	     unsigned long arg6)
{
	struct task *cur;
	long ret;

	cur = current_task();
	if (cur->state != TASK_RUNNING)
		BUG();

	switch (no) {
		case SYS_read:
			ret = sys_read(arg1, (char *)arg2, arg3);
			break;

		case SYS_open:
			ret = sys_open((const char *)arg1, arg2);
			break;

		case SYS_close:
			ret = sys_close(arg1);
			break;

		case SYS_stat:
			ret = sys_stat((const char *)arg1, (struct stat *)arg2);
			break;

		case SYS_write:
			ret = sys_write(arg1, (const char *)arg2, arg3);
			break;

		case SYS_brk:
			ret = sys_brk(arg1);
			break;

		case SYS_getpid:
			ret = sys_getpid();
			break;

		case SYS_fork:
			ret = sys_fork();
			break;

		case SYS_wait:
			ret = sys_wait(arg1, (void __user *)arg2, arg3);
			break;

		case SYS_sched_yield:
			ret = sys_sched_yield();
			break;

		case SYS_execve:
			ret = sys_execve((void *)arg1, (void *)arg2,
					 (void *)arg3);
			break;

		case SYS_exit:
			ret = sys_exit(arg1);
			break;

		case SYS_getdents:
			ret = sys_getdents(arg1, (void *)arg2, arg3);
			break;

		/* custom grinch syscalls */
		case SYS_grinch_usleep:
			ret = sys_grinch_usleep(arg1);
			break;

		case SYS_grinch_gettime:
			ret = sys_grinch_gettime();
			break;

		case SYS_grinch_create_grinch_vm:
			ret = sys_grinch_create_grinch_vm();
			break;

		case SYS_grinch_kstat:
			ret = sys_grinch_kstat(arg1, arg2);
			break;

		default:
			ret = -ENOSYS;
			break;
	}

	/*
	 * 1. On errors, always set the return value
	 * 2. We have special treatments for exit,execve,wait
	 * 3. Syscalls might have killed the task. Check for its existence
	 */
	cur = current_task();
	if ((ret < 0 ||
	    !(no == SYS_exit || no == SYS_execve || no == SYS_wait)) && cur)
		regs_set_retval(&cur->regs, ret);
}
