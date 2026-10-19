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

#ifndef _GRINCH_REBOOT_H
#define _GRINCH_REBOOT_H

#include <grinch/compiler_attributes.h>

/*
 * How much a method knows about the machine it ends. Several may be found
 * during init, in an order nobody should have to rely on, so the more
 * specific one wins no matter who installs first.
 */
enum reboot_prio {
	REBOOT_PRIO_FIRMWARE,	/* SBI, PSCI: whatever the machine offers */
	REBOOT_PRIO_PLATFORM,	/* a device that says how this machine ends */
	REBOOT_PRIO_GUEST,	/* we are not the machine; ask the host */
};

/*
 * Machine power-off / reset methods. An arch or driver installs one during
 * init; it returns an error only on failure -- a successful call never
 * returns.
 */
void shutdown_method_set(int (*method)(int err), enum reboot_prio prio);
void reboot_method_set(int (*method)(void), enum reboot_prio prio);

/* Returns only if no method took effect. */
void shutdown_attempt(int err);

void __noreturn shutdown(int err);
void __noreturn reboot(void);

#endif /* _GRINCH_REBOOT_H */
