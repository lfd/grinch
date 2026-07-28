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
 * Machine power-off / reset hooks. An arch or driver installs these during
 * init; each returns an error only on failure -- a successful call never
 * returns. A NULL hook means no method is available.
 */
extern int (*arch_shutdown)(int err);
extern int (*arch_reboot)(void);

void __noreturn shutdown(int err);
void __noreturn reboot(void);

#endif /* _GRINCH_REBOOT_H */
