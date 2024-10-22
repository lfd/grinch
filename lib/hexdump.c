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
#include <grinch/vsprintf.h>

static void printer(char **dst, const char *fmt, ...)
{
	int written;
	va_list ap;

	va_start(ap, fmt);
	written = vsnprintf(*dst, 32, fmt, ap);
	va_end(ap);

	if (written < 0)
		return;

	*dst += written;
}

void hexdump(const void *addr, unsigned long size)
{
	const unsigned char* p = (const unsigned char*)addr;
	unsigned long i, j;
	char line[96]; /* suffices for all cases */
	char *dst;

	for (i = 0; i < size; i += 16) {
		dst = line;
		printer(&dst, "%08lx: ", i);

		for (j = 0; j < 16; j++) {
			if (i + j < size)
				printer(&dst, "%02x ", p[i + j]);
			else
				printer(&dst, "   ");

			if (j % 8 == 7)
				printer(&dst, " ");
		}

		printer(&dst, " ");
		for (j = 0; j < 16; j++) {
			if (i + j < size)
				printer(&dst, "%c", isprint(p[i + j]) ? p[i + j] : '.');
			else
				printer(&dst, " ");
		}

		printer(&dst, "\n");
		printk("%s", line);
	}
}
