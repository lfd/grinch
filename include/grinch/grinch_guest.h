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

#ifndef _GRINCH_GUEST_H
#define _GRINCH_GUEST_H

#include <grinch/types.h>

#ifdef CONFIG_GRINCH_GUEST

extern bool grinch_is_guest;

/* Look for a hypervisor and, if one answers, become its guest. */
void guest_init(void);

#else /* !CONFIG_GRINCH_GUEST */

#define grinch_is_guest		false

static inline void guest_init(void) { }

#endif /* CONFIG_GRINCH_GUEST */

#endif /* _GRINCH_GUEST_H */
