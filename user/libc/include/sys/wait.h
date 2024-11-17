/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _WAIT_H
#define _WAIT_H

#include <grinch/types.h>

#define WTERMSIG(status)	((status) & 0x7f)
#define WEXITSTATUS(status)	(((status) & 0xff00) >> 8)
#define WIFEXITED(status)	(WTERMSIG(status) == 0)

pid_t waitpid(pid_t pid, int *wstatus, int options);

static inline pid_t wait(int *wstatus)
{
	return waitpid(-1, wstatus, 0);
}

#endif /* _WAIT_H */
