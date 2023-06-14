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

#ifndef _SYSCALL_H
#define _SYSCALL_H

unsigned long syscall(unsigned long no, unsigned long arg0,
		      unsigned long arg1, unsigned long arg2,
		      unsigned long arg3, unsigned long arg4,
		      unsigned long arg5);

#endif /* _SYSCALL_H */
