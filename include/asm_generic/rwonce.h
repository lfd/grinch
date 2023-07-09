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

/* partly copied from the Linux kernel */

#ifndef _RWONCE_H
#define _RWONCE_H

#include <grinch/compiler_types.h>

#define WRITE_ONCE(x, val)						\
do {									\
	*(volatile typeof(x) *)&(x) = (val);				\
} while (0)

#define READ_ONCE(x)	(*(const volatile __unqual_scalar_typeof(x) *)&(x))

#endif /* _RWONCE_H */
