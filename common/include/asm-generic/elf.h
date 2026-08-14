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

#ifndef _ASM_GENERIC_ELF_H
#define _ASM_GENERIC_ELF_H

#define PF_X	0x1
#define PF_W	0x2
#define PF_R	0x4

#ifdef LINKER_SCRIPT
PHDRS {
	text	PT_LOAD FLAGS(PF_R | PF_X);
	rodata	PT_LOAD FLAGS(PF_R);
	data	PT_LOAD FLAGS(PF_R | PF_W);
#ifdef PHDRS_DYNAMIC
	/* Only a relocatable image carries the table that describes it. */
	dynamic	PT_DYNAMIC FLAGS(PF_R);
#endif
}
#endif /* LINKER_SCRIPT */

#endif /* _ASM_GENERIC_ELF_H */
