/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define ARCH_PER_CPU_FIELDS				\
	union {						\
		unsigned char stack[STACK_SIZE];	\
		struct {				\
			unsigned char __fill[STACK_SIZE - sizeof(struct registers)];	\
			struct registers regs;		\
		};					\
	} exception;					\
	struct {					\
		u16 ctx;				\
	} plic;
