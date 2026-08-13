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

#ifndef _HYPERCALL_H
#define _HYPERCALL_H

#include <grinch/errno.h>

/*
 * Call numbers of the grinch hypercall interface. Both ends need them, so
 * they are declared whether or not this kernel can issue a hypercall.
 */
#define GRINCH_HYPERCALL_PRESENT		0x1
#define GRINCH_HYPERCALL_YIELD			0x2
#define GRINCH_HYPERCALL_VMQUIT			0x3
#define GRINCH_HYPERCALL_BP			0x4

#ifdef CONFIG_HAVE_HYPERCALL

#include <asm/hypercall.h>

#else /* !CONFIG_HAVE_HYPERCALL */

static inline int hypercall(unsigned long no, unsigned long arg1)
{
	return -ENOSYS;
}

#endif /* CONFIG_HAVE_HYPERCALL */

#define DEFINE_HYPERCALL_0(name, no)		\
static inline int hypercall_##name(void)	\
{						\
	return hypercall(no, 0);		\
}

#define DEFINE_HYPERCALL_1(name, no)			\
static inline int hypercall_##name(unsigned long arg)	\
{							\
	return hypercall(no, arg);			\
}

DEFINE_HYPERCALL_0(present, GRINCH_HYPERCALL_PRESENT)
DEFINE_HYPERCALL_0(yield, GRINCH_HYPERCALL_YIELD)
DEFINE_HYPERCALL_0(bp, GRINCH_HYPERCALL_BP)
DEFINE_HYPERCALL_1(vmquit, GRINCH_HYPERCALL_VMQUIT)

#endif /* _HYPERCALL_H */
