/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2025
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _MATH_H
#define _MATH_H

#define __round_mask(x, y)	((__typeof__(x))((y)-1))
#define round_down(x, y)	((x) & ~__round_mask(x, y))

#endif /* _MATH_H */
