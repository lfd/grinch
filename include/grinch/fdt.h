/*
 * Grinch, a minimalist RISC-V operating system
 *
 * Copyright (c) OTH Regensburg, 2022
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _FDT_H
#define _FDT_H

#include <libfdt.h>

extern unsigned char *_fdt;

int fdt_init(paddr_t pfdt);

int fdt_read_reg(const void *fdt, int nodeoffset, int idx, void *addrp,
		 u64 *sizep);

int fdt_probe_known(void *fdt, const char **names, unsigned int length);
bool fdt_device_is_available(const void *blob, unsigned long node);

#endif /* _FDT_H */
