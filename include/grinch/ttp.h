/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Lim Ern You <ern.lim@st.oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/types.h>

#ifndef _TTP_H
#define _TTP_H

struct ttp_storage {
	bool print_max;
	unsigned long long eventcount;
	struct ttp_event *events;
};

int gcall_ttp(unsigned int no);

int ttp_init(void);

int ttp_emit(unsigned int id);

#endif /* _TTP_H */
