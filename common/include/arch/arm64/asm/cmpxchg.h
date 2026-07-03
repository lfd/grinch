/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef __ASM_CMPXCHG_H
#define __ASM_CMPXCHG_H

#include <asm/atomic_ll_sc.h>

#define arch_cmpxchg_relaxed(ptr, o, n) \
	__ll_sc_cmpxchg_relaxed(ptr, o, n)

#endif /* __ASM_CMPXCHG_H */
