/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2026
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define dbg_fmt(x)	"asid: " x

#include <asm/spinlock.h>

#include <grinch/alloc.h>
#include <grinch/asid.h>
#include <grinch/bitmap.h>
#include <grinch/errno.h>
#include <grinch/init.h>
#include <grinch/paging.h>
#include <grinch/printk.h>

/*
 * ASIDs come from a global bitmap sized by the number of ASIDs the
 * architecture provides. ASID 0 is reserved as the untagged fallback
 * and never handed out.
 */
static DEFINE_SPINLOCK(asid_lock);
static unsigned long *asid_bitmap;
static unsigned long nr_asids;

int __init asid_init(void)
{
	nr_asids = arch_nr_asids();
	pri("ASIDs: %lu\n", nr_asids);

	/* Only ASID 0 available: every address space runs untagged. */
	if (nr_asids <= 1)
		return 0;

	asid_bitmap = bitmap_zalloc(nr_asids);
	if (!asid_bitmap)
		return -ENOMEM;

	/* ASID 0 is never handed out */
	bitmap_set(asid_bitmap, 0, 1);

	return 0;
}

unsigned long asid_alloc(void)
{
	unsigned long asid;

	/* No bitmap: only the untagged ASID 0 is available. */
	if (!asid_bitmap)
		return 0;

	spin_lock(&asid_lock);
	asid = bitmap_find_next_zero_area(asid_bitmap, nr_asids, 0, 1, 0);
	if (asid >= nr_asids) {
		asid = 0;
		goto unlock_out;
	}
	bitmap_set(asid_bitmap, asid, 1);

unlock_out:
	spin_unlock(&asid_lock);

	return asid;
}

void asid_free(unsigned long asid)
{
	if (!asid)
		return;

	/*
	 * Scrub every CPU before the ASID becomes available again: address
	 * space switches no longer flush, so a reused ASID must not inherit
	 * its predecessor's translations.
	 */
	flush_tlb_asid(asid);

	spin_lock(&asid_lock);
	bitmap_clear(asid_bitmap, asid, 1);
	spin_unlock(&asid_lock);
}
