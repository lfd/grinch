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

#ifndef _GRINCH_STRTOX_H
#define _GRINCH_STRTOX_H

unsigned long int strtoul(const char *cp, char **endptr, unsigned int base);

#endif /* _GRINCH_STRTOX_H */
