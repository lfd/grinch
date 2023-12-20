/*
 * Grinch, a minimalist operating system
 *
 * Copyright (c) OTH Regensburg, 2022-2023
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* Taken and adapted from: */

/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013-2022
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/spinlock.h>

#include <stdarg.h>

#include <grinch/string.h>
#include <grinch/boot.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/serial.h>
#include <grinch/math64.h>

static DEFINE_SPINLOCK(print_lock);
static char prefix_fmt[16] = "[Grinch %u] ";

struct {
	unsigned int tail;
	char content[2048];
} console;

static void __vprintk(const char *fmt, va_list ap);

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

static void __printf(1, 2) _printk_raw(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	__vprintk(fmt, ap);
	va_end(ap);
}

static inline void print_prefix(void)
{
	_printk_raw(prefix_fmt, grinch_id);
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

static inline void __puts(const char *msg, bool prefixed)
{
	spin_lock(&print_lock);
	if (prefixed)
		print_prefix();
	___puts(msg);
	spin_unlock(&print_lock);
}

void puts(const char *msg)
{
	__puts(msg, true);
}

void _puts(const char *msg)
{
	__puts(msg, false);
}

static char *uint2str(unsigned long long value, char *buf)
{
	unsigned long long digit, divisor = 10000000000000000000ULL;
	int first_digit = 1;

	while (divisor > 0) {
		digit = div_u64_u64(value, divisor);
		value -= digit * divisor;
		if (!first_digit || digit > 0 || divisor == 1) {
			*buf++ = '0' + digit;
			first_digit = 0;
		}
		divisor = div_u64_u64(divisor, 10);
	}

	return buf;
}

static char *int2str(long long value, char *buf)
{
	if (value < 0) {
		*buf++ = '-';
		value = -value;
	}
	return uint2str(value, buf);
}

static char *hex2str(unsigned long long value, char *buf,
		     unsigned long long leading_zero_mask)
{
	static const char hexdigit[] = "0123456789abcdef";
	unsigned long long digit, divisor = 0x1000000000000000ULL;
	int first_digit = 1;

	while (divisor > 0) {
		digit = div_u64_u64(value, divisor);
		value -= digit * divisor;
		if (!first_digit || digit > 0 || divisor == 1 ||
		    divisor & leading_zero_mask) {
			*buf++ = hexdigit[digit];
			first_digit = 0;
		}
		divisor >>= 4;
	}

	return buf;
}

static char *align(char *p1, char *p0, unsigned int width, char fill)
{
	unsigned int n;

	/* Note: p1 > p0 here */
	if ((unsigned int)(p1 - p0) >= width)
		return p1;

	for (n = 1; p1 - n >= p0; n++)
		*(p0 + width - n) = *(p1 - n);
	memset(p0, fill, width - (p1 - p0));
	return p0 + width;
}

static void __vprintk(const char *fmt, va_list ap)
{
	char buf[128];
	char *p, *p0;
	char c, fill;
	unsigned long long v;
	unsigned int width;
	enum {SZ_NORMAL, SZ_LONG, SZ_LONGLONG} length;

	p = buf;

	while (1) {
		c = *fmt++;
		if (c == 0) {
			break;
		} else if (c == '%') {
			*p = 0;
			___puts(buf);
			p = buf;

			c = *fmt++;

			width = 0;
			p0 = p;
			fill = (c == '0') ? '0' : ' ';
			while (c >= '0' && c <= '9') {
				width = width * 10 + c - '0';
				c = *fmt++;
				if (width >= sizeof(buf) - 1)
					width = 0;
			}

			length = SZ_NORMAL;
			if (c == 'l') {
				length = SZ_LONG;
				c = *fmt++;
				if (c == 'l') {
					length = SZ_LONGLONG;
					c = *fmt++;
				}
			}

			switch (c) {
			case 'c':
				*p++ = (unsigned char)va_arg(ap, int);
				break;
			case 'd':
				if (length == SZ_LONGLONG)
					v = va_arg(ap, long long);
				else if (length == SZ_LONG)
					v = va_arg(ap, long);
				else
					v = va_arg(ap, int);
				p = int2str(v, p);
				p = align(p, p0, width, fill);
				break;
			case 'p':
				*p++ = '0';
				*p++ = 'x';
				v = va_arg(ap, unsigned long);
				p = hex2str(v, p, (unsigned long)-1);
				break;
			case 's':
				___puts(va_arg(ap, const char *));
				break;
			case 'u':
			case 'x':
				if (length == SZ_LONGLONG)
					v = va_arg(ap, unsigned long long);
				else if (length == SZ_LONG)
					v = va_arg(ap, unsigned long);
				else
					v = va_arg(ap, unsigned int);
				if (c == 'u')
					p = uint2str(v, p);
				else
					p = hex2str(v, p, 0);
				p = align(p, p0, width, fill);
				break;
			default:
				*p++ = '%';
				*p++ = c;
				break;
			}
		} else {
			*p++ = c;
		}
		if (p >= &buf[sizeof(buf) - 1]) {
			*p = 0;
			___puts(buf);
			p = buf;
		}
	}

	*p = 0;
	___puts(buf);
}

void __printf(1, 2) printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	spin_lock(&print_lock);
	print_prefix();
	__vprintk(fmt, ap);
	spin_unlock(&print_lock);
	va_end(ap);
}

void __noreturn __printf(1, 2) panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	spin_lock(&print_lock);
	___puts("KERNEL PANIC: ");
	__vprintk(fmt, ap);
	spin_unlock(&print_lock);
	va_end(ap);

	panic_stop();
}

void printk_init(void)
{
	if (grinch_is_guest)
		strncpy(prefix_fmt, "[GrinchVM %02u] ", sizeof(prefix_fmt));
}
