/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2024
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <ctype.h>

#include <grinch/boot.h>
#include <grinch/cpu.h>
#include <grinch/bootparam.h>
#include <grinch/console.h>
#include <grinch/minmax.h>
#include <grinch/printk.h>
#include <grinch/string.h>
#include <grinch/timer.h>
#include <grinch/vsprintf.h>


#ifdef DEBUG
#define LOGLEVEL_DEFAULT	9
#else
#define LOGLEVEL_DEFAULT	2
#endif

static DEFINE_SPINLOCK(print_lock);
static char prefix_fmt[32];
static unsigned int loglevel = LOGLEVEL_DEFAULT;

static void __init loglevel_parse(const char *arg)
{
	unsigned int level;

	level = min(strtoul(arg, NULL, 10), 10UL);
	loglevel_set(level);
}
bootparam(loglevel, loglevel_parse);

void loglevel_set(unsigned int level)
{
	loglevel = level;
}

static int sprint_prefix(char **str, char *end)
{
	struct timespec ts;
	int res;

	timer_get_wall(&ts);
	res = snprintf(*str, end - *str, prefix_fmt, PR_TS_PARAMS(&ts));
	if (res < 0)
		return res;

	*str += res;

	return res;
}

void _puts(const char *msg)
{
	spin_lock(&print_lock);
	console_puts(msg);
	spin_unlock(&print_lock);
}

void vprintk(const char *fmt, const char *infix, va_list ap)
{
	char *str, *end;
	char buf[196];
	bool prefix;
	int err;
	unsigned int this_loglevel;

	this_loglevel = 0;
	prefix = true;
	while (*fmt == PR_SOH_ASCII) {
		fmt++;
		if (!*fmt)
			return;

		if (*fmt == PR_NOPREFIX_ASCII)
			prefix = false;

		if (isdigit(*fmt))
			this_loglevel = *fmt - '0';

		fmt++;
	}

	if (this_loglevel > loglevel)
		return;

	str = buf;
	end = str + sizeof(buf);

	if (prefix) {
		err = sprint_prefix(&str, end);
		if (err < 0)
			return;
	}

	if (infix) {
		err = snprintf(str, end - str, "%s", infix);
		if (err < 0)
			return;
		str += err;
	}

	err = vsnprintf(str, end - str, fmt, ap);
	if (err < 0)
		return;

	//str += err;

	_puts(buf);
}

void __printf(1, 2) printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, NULL, ap);
	va_end(ap);
}

void __init printk_init(void)
{
	snprintf(prefix_fmt, sizeof(prefix_fmt), ISTR("[Grinch%s %u %s] "),
			grinch_is_guest ? ISTR("VM") : ISTR(""), grinch_id,
			PR_TS_FMT);
}
