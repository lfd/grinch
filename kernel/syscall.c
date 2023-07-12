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

#define dbg_fmt(x)	"task: " x

#include <grinch/errno.h>
#include <grinch/syscall.h>
#include <grinch/syscall_common.h>
#include <grinch/printk.h>
#include <grinch/percpu.h>
#include <grinch/task.h>
#include <grinch/uaccess.h>

static unsigned long sys_write(int fd, const char *buf, size_t count)
{
#define BLEN	63

	char tmp[BLEN + 1];
	unsigned long sz;
	int err;

	if (fd != 1)
		return -ENOENT;

	while (count) {
		sz = count < BLEN ? count : BLEN;
		err = copy_from_user(&current_task()->mm, tmp, buf, sz);
		if (err < 0)
			return err;
		tmp[sz] = 0;
		printk("%s", tmp);

		count -= err;
		buf += err;
	}

	return 0;
}

static void exit(int code)
{
	struct task *task;

	task = current_task();

	pr("PID %u exited: %d\n", task->pid, code);
	sched_dequeue(task);
	schedule();
	task_destroy(task);
}

int syscall(unsigned long no, unsigned long arg1,
	    unsigned long arg2, unsigned long arg3,
	    unsigned long arg4, unsigned long arg5,
	    unsigned long arg6)
{
	unsigned long ret;

	switch (no) {
		case SYS_write:
			ret = sys_write(arg1, (const char *)arg2, arg3);
			break;

		case SYS_getpid:
			ret = current_task()->pid;
			break;

		case SYS_fork:
			ret = do_fork();
			break;

		case SYS_sched_yield:
			this_per_cpu()->schedule = true;
			ret = 0;
			break;

		case SYS_execve:
			ret = do_execve((void *)arg1, (void *)arg2, (void *)arg3);
			if (ret) {
				pr("execve failed on task %u. Exiting.\n", current_task()->pid);
				exit(ret);
			}
			break;

		case SYS_exit:
			exit(arg1);
			break;

		default:
			ret = -ENOSYS;
			break;
	}

	if (no != SYS_exit && no != SYS_execve)
		regs_set_retval(&current_task()->regs, ret);

	return 0;
}
