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

#define dbg_fmt(x)	"reboot: " x

#include <grinch/errno.h>
#include <grinch/init.h>
#include <grinch/panic.h>
#include <grinch/printk.h>
#include <grinch/reboot.h>
#include <grinch/syscall.h>
#include <grinch/reboot_abi.h>

static struct {
	int (*method)(int err);
	enum reboot_prio prio;
} shutdown_method;

static struct {
	int (*method)(void);
	enum reboot_prio prio;
} reboot_method;

void __init shutdown_method_set(int (*method)(int err), enum reboot_prio prio)
{
	if (shutdown_method.method && shutdown_method.prio > prio)
		return;

	shutdown_method.method = method;
	shutdown_method.prio = prio;
}

void __init reboot_method_set(int (*method)(void), enum reboot_prio prio)
{
	if (reboot_method.method && reboot_method.prio > prio)
		return;

	reboot_method.method = method;
	reboot_method.prio = prio;
}

void shutdown_attempt(int err)
{
	if (shutdown_method.method)
		shutdown_method.method(err);
}

void __noreturn shutdown(int err)
{
	pr("Shutdown. Reason: %pe\n", ERR_PTR(err));

	if (!shutdown_method.method)
		panic("No shutdown method\n");

	err = shutdown_method.method(err);
	panic("Shutdown failed: %pe\n", ERR_PTR(err));
}

void __noreturn reboot(void)
{
	int err;

	if (!reboot_method.method)
		panic("No reboot method\n");

	err = reboot_method.method();
	panic("Reboot failed: %pe\n", ERR_PTR(err));
}

SYSCALL_DEF2(reboot, int, magic, unsigned int, cmd)
{
	if (magic != GRINCH_REBOOT_MAGIC)
		return -EINVAL;

	switch (cmd) {
	case GRINCH_REBOOT_CMD_HALT:
		shutdown(0);
	case GRINCH_REBOOT_CMD_REBOOT:
		reboot();
	default:
		return -EINVAL;
	}
}
