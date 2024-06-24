/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2024
 *
 * Authors:
 *  Ern Lim <ern.lim@st.oth-regensburg.de>
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _GRINCH_GCOV_H
#define _GRINCH_GCOV_H

#ifdef GCOV

extern struct gcov_min_info *gcov_info_head;
#define GCOV_HEAD	&gcov_info_head

void gcov_init(void);

#else /* !GCOV */

#define GCOV_HEAD	NULL

static inline void gcov_init(void)
{
}

#endif /* GCOV */

#endif /* _GRINCH_GCOV_H */
