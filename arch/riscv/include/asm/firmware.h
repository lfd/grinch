/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef _FIRMWARE_H
#define _FIRMWARE_H

#include <grinch/types.h>

/*
 * Services the kernel cannot reach on its own privilege level. A supervisor
 * kernel asks firmware for them, a machine mode kernel is the firmware and
 * drives the hardware itself.
 */

/* Probe the interface. Fails if it cannot serve what the kernel needs. */
int firmware_init(void);

/* Program the next timer expiry, as an absolute value in timebase ticks. */
void firmware_timer_set(u64 ticks);

/* Raise a software interrupt on the given harts. */
void firmware_ipi_send(unsigned long hmask);

/* Start a hart at the given physical address. */
int firmware_hart_start(unsigned long hart_id, paddr_t entry,
			unsigned long opaque);

/* Fence remote harts. A zero size covers the entire address space. */
void firmware_remote_fence(unsigned long hmask, const void *addr, size_t size);
void firmware_remote_fence_asid(unsigned long hmask, unsigned long asid,
				const void *addr, size_t size);

#endif /* _FIRMWARE_H */
