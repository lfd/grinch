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

#ifndef _SYSCALL_COMMON_H
#define _SYSCALL_COMMON_H

#define SYS_read		0
#define SYS_write		1
#define SYS_open		2
#define SYS_close		3
#define SYS_sched_yield		24
#define SYS_getpid		39
#define SYS_fork		57
#define SYS_execve		59
#define SYS_exit		60
#define SYS_wait		260
#define SYS_usleep		1337
#define SYS_gettime		1338
#define SYS_create_grinch_vm	1339

#endif /* _SYSCALL_COMMON_H */
