/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _PSCI_H
#define _PSCI_H

#include <grinch/types.h>

/* Discover the PSCI conduit from the device tree. */
void psci_init(void);

/* Power on a secondary CPU; -ENODEV if no PSCI conduit was discovered. */
int psci_cpu_on(unsigned long mpidr, paddr_t entry, unsigned long context);

#endif /* _PSCI_H */
