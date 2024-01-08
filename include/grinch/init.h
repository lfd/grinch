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

#define __init			__attribute__((section(".init.text"), used))
#define __initconst		__attribute__((section(".init.rodata"), used))
#define __initbootparams	__attribute__((section(".init.bootparams"), used))

#endif /* _INIT_H */
