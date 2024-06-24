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

/* This file is copied and adapted from the Jailhouse project
 *
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2017
 *
 * Authors:
 *  Henning Schild <henning.schild@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <grinch/errno.h>
#include <grinch/gcov.h>
#include <grinch/symbols.h>

struct gcov_min_info *gcov_info_head;

/*
 * the actual data structure is bigger but we just need to know the version
 * independent beginning to link the elements to a list.
 */
struct gcov_min_info {
	unsigned int version;
	struct gcov_min_info *next;
};

void gcov_init(void)
{
	unsigned long *iarray;
	void (*func)(void);

	for (iarray = __init_array_start; iarray < __init_array_end; iarray++) {
		func = (void *)iarray[0];
		func();
	}
}

void __gcov_init(struct gcov_min_info *info);
void __gcov_init(struct gcov_min_info *info)
{
	info->next = gcov_info_head;
	gcov_info_head = info;
}

/* Satisfy the linker, never called */
void __gcov_merge_add(void *counters, unsigned int n_counters);
void __gcov_merge_add(void *counters, unsigned int n_counters)
{
}
