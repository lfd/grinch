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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <grinch/vsprintf.h>

static int _puts(const char *s)
{
	ssize_t ret;

	ret = write(stdout, s, strlen(s));

	return ret;
}

static int sprint_prefix(char **str, char *end)
{
	int res;

	res = snprintf(*str, end - *str, __app_name_fmt, getpid());
	if (res < 0)
		return res;

	*str += res;

	return res;
}

static int vprintf(const char *fmt, va_list ap)
{
	char buf[196];
	char *str, *end;
	int err;

	str = buf;
	end = str + sizeof(buf);

	err = sprint_prefix(&str, end);
	if (err < 0)
		return err;

	err = vsnprintf(str, end - str, fmt, ap);
	if (err < 0)
		return err;

	//str += err;

	_puts(buf);

	return str - buf;
}

int __printf(1, 2) printf(const char *fmt, ...)
{
	va_list ap;
	int err;

	va_start(ap, fmt);
	err = vprintf(fmt, ap);
	va_end(ap);

	return err;
}

int puts(const char *s)
{
	int err;

	err = printf("%s", s);
	if (err < 0)
		return EOF;

	return err;
}
