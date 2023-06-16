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

#ifndef _VMA_H
#define _VMA_H

/* Virtual Memory Area: Userspace / Kernelspace VMA areas outside KMMM */
int vma_init_fdt(void);
int vma_init(paddr_t addrp, size_t sizep);

#endif /* _VMA_H */
