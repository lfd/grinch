/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _ISA_H

#ifndef __ASSEMBLY__

#include <grinch/types.h>

static inline bool has_hypervisor(void)
{
	return false;
}

#endif /* __ASSEMBLY__ */

#endif /* _ISA_H */
