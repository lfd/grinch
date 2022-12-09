/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Stefan Huber <stefan.huber@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _BUG_H
#define _BUG_H

#include <grinch/cpu.h>
#include <grinch/irq.h>
#include <grinch/printk.h>

#define BUG(x)							\
do {								\
	pr("BUG: found at %s:%d on CPU %lu :-(\n",		\
	   __FILE__, __LINE__, this_cpu_id());			\
	pr("BUG: " x);						\
	ext_disable();						\
	pr("BUG: System halted\n");				\
	cpu_halt();						\
} while (0)

#endif /* _BUG_H */
