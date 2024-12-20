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

#ifndef _KINFO_ABI
#define _KINFO_ABI

#include <grinch/time_abi.h>

struct kinfo {
	timeu_t wall_base;
	union {
		struct {
			u32 timebase_frequency;
		} riscv;
	};
};

#endif /* _KINFO_ABI */
