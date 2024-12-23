/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
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
#include <grinch/iores.h>

struct device;

struct of_device_id {
	char compatible[32];
	const void *data;
};

extern unsigned char *_fdt;

int fdt_init(paddr_t pfdt);

int fdt_read_reg(const void *fdt, int nodeoffset, int idx,
		 struct mmio_area *mmio);

int fdt_addr_sz(const void *fdt, int off, int *_ac, int *_sc);

int fdt_read_u64(const void *fdt, int nodeoffset, const char *name, u64 *res);
static inline int
fdt_read_u32(const void *fdt, int nodeoffset, const char *name, u32 *res)
{
	u64 tmp;
	int err;

	err = fdt_read_u64(fdt, nodeoffset, name, &tmp);
	if (!err)
		*res = tmp;

	return err;
}

int fdt_read_u32_array(const void *fdt, int nodeoffset, const char *name,
		       u32 *array, size_t min, size_t len);

bool fdt_device_is_available(const void *fdt, unsigned long node);

int fdt_find_device(const void *fdt, const char *path,
		    const struct of_device_id *compats,
		    const struct of_device_id **match);

int fdt_match_device_off(const void *fdt, int offset,
			 const struct of_device_id *compats,
			 const struct of_device_id **match);

int fdt_irq_get(struct device *dev);

/* FDT manipulation */
int fdt_property_reg_u64_simple(void *fdt, const char *name, u64 addr, u64 sz);

#endif /* _FDT_H */
