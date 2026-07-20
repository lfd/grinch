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

#ifndef _ASID_H
#define _ASID_H

/*
 * Every address space gets its own ASID, so that its translations are
 * tagged apart in the TLB. ASID 0 is shared by the kernel and serves as
 * the untagged fallback: it is returned on hardware without ASIDs, and
 * when the allocator runs dry.
 */
int asid_init(void);
unsigned long asid_alloc(void);
void asid_free(unsigned long asid);

/*
 * Number of ASIDs the architecture provides, including the reserved
 * ASID 0. One means the hardware cannot tag translations apart, so
 * everything runs untagged.
 */
unsigned long arch_nr_asids(void);

#endif /* _ASID_H */
