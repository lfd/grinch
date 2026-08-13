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

#include <grinch/boot.h>
#include <grinch/grinch_guest.h>
#include <grinch/hypercall.h>
#include <grinch/init.h>
#include <grinch/reboot.h>

bool grinch_is_guest;

static int guest_shutdown(int err)
{
	return hypercall_vmquit(err);
}

static int guest_reboot(void)
{
	/* No reboot hypercall yet -- fall back to halt */
	return hypercall_vmquit(0);
}

void __init guest_init(void)
{
	int ret;

	ret = hypercall_present();
	if (ret <= 0)
		return;

	grinch_is_guest = true;
	grinch_id = ret;

	arch_shutdown = guest_shutdown;
	arch_reboot = guest_reboot;
}
