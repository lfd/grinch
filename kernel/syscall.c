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

#include <grinch/alloc.h>
#include <grinch/errno.h>
#include <grinch/kstat.h>
#include <grinch/syscall.h>
#include <grinch/syscall_common.h>
#include <grinch/printk.h>
#include <grinch/percpu.h>
#include <grinch/task.h>
#include <grinch/timer.h>
#include <grinch/uaccess.h>

static unsigned long usleep(unsigned long us)
{
	task_sleep_for(current_task(), US_TO_NS(us));
	this_per_cpu()->schedule = true;

	return 0;
}

static int grinch_kstat(unsigned long no, unsigned long arg)
{
	int ret;

	ret = 0;
	switch (no) {
		case GRINCH_KSTAT_PS:
			tasks_dump();
			break;

		case GRINCH_KSTAT_KHEAP:
			kheap_stats();
			break;

		case GRINCH_KSTAT_MAPS:
			process_show_vmas(arg);
			break;

		default:
			ret = -ENOSYS;
			break;
	}

	return ret;
}

int syscall(unsigned long no, unsigned long arg1,
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
			ret = cur->pid;
			break;

		case SYS_fork:
			ret = do_fork();
			break;

		case SYS_wait:
			ret = task_wait(arg1, (void __user *)arg2, arg3);
			if (ret < 0)
				regs_set_retval(&cur->regs, ret);
			break;

		case SYS_sched_yield:
			this_per_cpu()->schedule = true;
			ret = 0;
			break;

		case SYS_execve:
			ret = sys_execve((void *)arg1, (void *)arg2,
					 (void *)arg3);
			if (ret) {
				pr("execve failed on task %u: %pe\n",
				   cur->pid, ERR_PTR(ret));
				task_exit(cur, ret);
			}
			break;

		case SYS_exit:
			task_exit(cur, arg1);
			ret = 0;
			break;

		case SYS_getdents:
			ret = sys_getdents(arg1, (void *)arg2, arg3);
			break;

		case SYS_usleep:
			ret = usleep(arg1);
			break;

		case SYS_gettime:
			ret = timer_get_wall();
			break;

		case SYS_create_grinch_vm:
			ret = vm_create_grinch();
			break;

		case SYS_grinch_kstat:
			ret = grinch_kstat(arg1, arg2);
			break;

		default:
			ret = -ENOSYS;
			break;
	}

	if (no != SYS_exit && no != SYS_execve && no != SYS_wait)
		regs_set_retval(&cur->regs, ret);

	return 0;
}
