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

/* Partly copied from Linux kernel sources */

#ifndef _ALIGN_H
#define _ALIGN_H

#define ALIGN_MASK(x, mask)		(((x) + (mask)) & ~(mask))

#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)
#define ALIGN(x, a)			ALIGN_MASK(x, (__typeof__(x))(a) - 1)
#define ALIGN_DOWN(x, a)		ALIGN((x) - ((a) - 1), (a))
#define PAGE_ALIGN_DOWN(addr)		ALIGN_DOWN(addr, PAGE_SIZE)
#define PAGE_ALIGNED(addr)		IS_ALIGNED(addr, PAGE_SIZE)

#define PTR_IS_ALIGNED(x, a)		(IS_ALIGNED((unsigned long)(x), (a)))
#define PTR_ALIGN(p, a)			((typeof(p))ALIGN((unsigned long)(p), (a)))
#define PTR_ALIGN_DOWN(p, a)		((typeof(p))ALIGN_DOWN((unsigned long)(p), (a)))
#define PTR_PAGE_ALIGN_DOWN(addr)	PTR_ALIGN_DOWN(addr, PAGE_SIZE)
#define PTR_PAGE_ALIGNED(addr)		PTR_IS_ALIGNED((unsigned long)(addr), PAGE_SIZE)


#endif /* _ALIGN_H */
