/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _INIT_H
#define _INIT_H

#include <grinch/compiler_attributes.h>

#define __init			__used __section(".init.text")
#define __initconst		__used __section(".init.rodata")
#define __initdata		__used __section(".init.data")
#define __initbootparams	__used __section(".init.bootparams")

/* Constant init string */
#define ISTR(X)			({static const char __c[] __initconst = (X); &__c[0];})

#endif /* _INIT_H */
