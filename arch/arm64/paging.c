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

#include <grinch/asid.h>
#include <grinch/paging.h>
#include <grinch/types.h>

void arch_paging_init(void) {}
void arch_paging_enable(unsigned long this_cpu, page_table_t pt) {}

unsigned long arch_nr_asids(void) { return 1; }
