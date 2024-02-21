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
static unsigned char vm[3] = {'V', 'M', 0};

struct {
	unsigned int tail;
	char content[2048];
} console;

static void __printf(1, 2) _printk_raw(const char *fmt, ...);

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

static inline void print_prefix(void)
{
	unsigned long long wall;

	wall = timer_get_wall();
	_printk_raw("[Grinch%s %u " PR_TIME_FMT "] ", vm, grinch_id, PR_TIME_PARAMS(wall));
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

static void vprintk(const char *fmt, va_list ap)
{
	// FIXME
	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, ap);
	___puts(buf);
	return;
}

static void __printf(1, 2) _printk_raw(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}

void __printf(1, 2) printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	spin_lock(&print_lock);
	print_prefix();
	vprintk(fmt, ap);
	spin_unlock(&print_lock);
	va_end(ap);
}

void __noreturn __printf(1, 2) panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	spin_lock(&print_lock);
	print_prefix();
	___puts(PANIC_PREFIX);
	vprintk(fmt, ap);
	spin_unlock(&print_lock);
	va_end(ap);

	do_panic();
}

void __init printk_init(void)
{
	if (!grinch_is_guest)
		vm[0] = 0;
}
