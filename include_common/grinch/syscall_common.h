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

#ifndef _SYSCALL_COMMON_H
#define _SYSCALL_COMMON_H

#define SYS_write		1
#define SYS_sched_yield		24
#define SYS_getpid		39
#define SYS_fork		57
#define SYS_execve		59
#define SYS_exit		60
#define SYS_usleep		1337

#endif /* _SYSCALL_COMMON_H */
