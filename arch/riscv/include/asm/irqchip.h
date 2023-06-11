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

extern const struct irqchip_fn irqchip_fn_plic;
extern const struct irqchip_fn irqchip_fn_aplic;

int plic_init(void *vaddr);
int aplic_init(void *vaddr);
