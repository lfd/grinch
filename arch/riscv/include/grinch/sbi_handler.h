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

#ifndef _SBI_HANDLER_H
#define _SBI_HANDLER_H

#include <asm/cpu.h>

int handle_ecall(struct registers *regs);

#endif /* _SBI_HANDLER_H */
