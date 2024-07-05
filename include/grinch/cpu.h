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

#ifndef _CPU_H
#define _CPU_H

#include <asm/cpu.h>

extern bool grinch_is_guest;

void arch_guest_init(void);

void dump_regs(struct registers *a);
void dump_exception(struct trap_context *ctx);

#endif /* _CPU_H */
