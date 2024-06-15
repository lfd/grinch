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

#ifndef _GRINCH_KSTAT_H
#define _GRINCH_KSTAT_H

#define GRINCH_KSTAT_PS		0 /* dump processes */
#define GRINCH_KSTAT_KHEAP	1 /* dump kheap stats */
#define GRINCH_KSTAT_MAPS	2 /* show user VMAs */
#define GRINCH_KSTAT_LSOF	3 /* show list of open files */
#define GRINCH_KSTAT_LSDEV	4 /* show list of devices */
#define GRINCH_KSTAT_LSPCI	5 /* list PCI devices */

#endif /* _GRINCH_KSTAT_H */
