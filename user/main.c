/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <syscall.h>

static inline void putc(char c)
{
	syscall(0, c, 0, 0, 0, 0, 0);
}

static void puts(const char *str)
{
	char c;

	while (1) {
		c = *str++;
		if (!c)
			break;
		putc(c);
	}
}

int main(void);

int main(void)
{
	puts("Hello, world from userspace!\n");
	return 0;
}
