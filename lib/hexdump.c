/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <ctype.h>

#include <grinch/hexdump.h>
#include <grinch/printk.h>

void hexdump(const void *addr, unsigned long size)
{
	const unsigned char* p = (const unsigned char*)addr;
	unsigned long i, j;

	for (i = 0; i < size; i += 16) {
		printk("%06lx: ", i);
		for (j = 0; j < 16; j++) {
			if (i + j < size)
				printk("%02x ", p[i + j]);
			else
				printk("   ");

			if (j % 8 == 7)
				printk(" ");
		}

		printk(" ");
		for (j = 0; j < 16; j++) {
			if (i + j < size)
				printk("%c", isprint(p[i + j]) ? p[i + j] : '.');
			else
				printk(" ");
		}

		printk("\n");
	}
}
