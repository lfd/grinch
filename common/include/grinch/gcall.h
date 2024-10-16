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

#ifndef _GRINCH_GCALL_H
#define _GRINCH_GCALL_H

#define GCALL_PS	0 /* dump processes */
#define GCALL_KHEAP	1 /* dump kheap stats */
#define GCALL_MAPS	2 /* show user VMAs */
#define GCALL_LSOF	3 /* show list of open files */
#define GCALL_LSDEV	4 /* show list of devices */
#define GCALL_LSPCI	5 /* list PCI devices */

#endif /* _GRINCH_GCALL_H */
