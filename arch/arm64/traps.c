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

#include <grinch/panic.h>
#include <grinch/percpu.h>

struct registers;
void arch_handle_exception(struct registers *regs);
void arch_handle_irq(struct registers *regs);

void arch_handle_exception(struct registers *regs) { panic("Unhandled exception\n"); }
void arch_handle_irq(struct registers *regs) { panic("Unhandled IRQ\n"); }
