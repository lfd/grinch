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

#include <asm/spinlock.h>

#include <grinch/boot.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/serial.h>
#include <grinch/timer.h>
#include <grinch/vsprintf.h>

static DEFINE_SPINLOCK(print_lock);
static char prefix_fmt[32];

struct {
	unsigned int tail;
	char content[2048];
} console;

static inline void console_write(char c)
{
	console.content[console.tail % sizeof(console.content)] = c;
	console.tail++;
}

void console_flush(void)
{
	unsigned int pos;

	if (console.tail > sizeof(console.content)) {
		for (pos = console.tail % sizeof(console.content); pos < sizeof(console.content); pos++)
			uart_write_char(&uart_default, console.content[pos]);
	}

	for (pos = 0; pos < console.tail % sizeof(console.content); pos++)
		uart_write_char(&uart_default, console.content[pos]);

	console.tail = 0;
}

static int sprint_prefix(char **str, char *end)
{
	unsigned long long wall;
	int res;

	wall = timer_get_wall();
	res = snprintf(*str, end - *str, prefix_fmt, PR_TIME_PARAMS(wall));
	if (res < 0)
		return res;

	*str += res;

	return res;
}

static void ___puts(const char *msg)
{
	char c;

	while (1) {
		c = *msg++;
		if (!c)
			break;

		uart_write_char(&uart_default, c);
		console_write(c);
	}
}

void _puts(const char *msg)
{
	spin_lock(&print_lock);
	___puts(msg);
	spin_unlock(&print_lock);
}

static void vprintk(const char *fmt, const char *infix, va_list ap)
{
	char buf[196];
	char *str, *end;
	int err;

	str = buf;
	end = str + sizeof(buf);

	err = sprint_prefix(&str, end);
	if (err < 0)
		return;

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

void __noreturn __printf(1, 2) panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, PANIC_PREFIX, ap);
	va_end(ap);

	do_panic();
}

void __init printk_init(void)
{
	snprintf(prefix_fmt, sizeof(prefix_fmt), ISTR("[Grinch%s %u %s] "),
			grinch_is_guest ? ISTR("VM") : ISTR(""), grinch_id,
			PR_TIME_FMT);
}
