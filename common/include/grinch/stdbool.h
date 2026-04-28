/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024-2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _GRINCH_STDBOOL_H
#define _GRINCH_STDBOOL_H

#if __STDC_VERSION__ < 202311L
typedef enum { true = 1, false = 0 } bool;
#endif

#endif /* _GRINCH_STDBOOL_H */
