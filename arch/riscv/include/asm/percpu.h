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

#define ARCH_PER_CPU_FIELDS				\
	struct {					\
		/*					\
		 * Offset of interrupt-controller	\
		 * phandle node in device-tree.		\
		 */					\
		int cpu_phandle;			\
		u16 ctx;				\
	} plic;
